namespace amu {


namespace tac {

// TODO(sushi) make it so that nop TAC are actually place holders and are simply replaced here
//             so that jumps actually jump to the next instruction
TAC*
add_tac(Gen* gen) {
    TAC* out = pool::add(gen->tac_pool);
    out->id = gen->tac_count++;
    out->bc_offset = -1;

    if(gen->resolve_breaks)
        out->jump_from = array::init<TAC*>();
    forI(gen->resolve_breaks) {
        Array<TAC*> stack = array::pop(gen->break_stacks);
        forI(stack.count) {
            TAC* t = array::read(stack, i);
            t->arg0 = out;
            array::push(out->jump_from, t);
        }
        array::deinit(stack);
    }

    gen->resolve_breaks = 0;

    return out;
}

b32 label(Label* l, u64 stack_offset);
void block(Code* code, Block* e);
Arg expression(Code* code, Expr* e);

// TODO(sushi) clean up conditional generation
//             we dont need an array of TAC for 'falselist', we just need to store the last cond_jump
struct ConditionalState {
    Array<TAC*> truelist; // list of cond_jumps that need to be backpatched with a TAC to jump to
    Array<TAC*> falselist; // list of plain jumps that need to be backpatched with a TAC To jump to
    Var* temp;
};

Arg
conditional(Code* code, Expr* cond, ConditionalState* state) {
    Arg condition = expression(code, cond->first_child<Expr>());

    TAC* cond_jump = add_tac(code->gen);
    cond_jump->op = tac::jump_zero;
    cond_jump->arg0 = condition;

    array::push(state->falselist, cond_jump);
    array::push(code->gen->tac, cond_jump);

    cond_jump->node = cond;

    Expr* first = cond->first_child()->next<Expr>();
    Expr* second = cond->last_child<Expr>();

    Arg f = expression(code, first);
    if(state->temp) {
        TAC* assign = add_tac(code->gen);
        assign->op = tac::assignment;
        assign->arg0.kind = arg::var;
        assign->arg0.var = state->temp;
        assign->arg1 = f;
        assign->node = first;
        array::push(code->gen->tac, assign);
    }

    if(second != first) {
        // add the if/else exit jump
        TAC* jump = add_tac(code->gen);
        jump->op = tac::jump;

        array::push(code->gen->tac, jump);

        // the last cond_jump can be resolved to the instruction following the jump 
        TAC* resolved = array::pop(state->falselist);
        u64 next_index = code->gen->tac.count;

        array::push(state->truelist, jump);
        jump->node = second;
        
        Arg s;
        if(second->kind == expr::conditional) s = conditional(code, second, state);
        else {
            s = expression(code, second);
            if(state->temp) {
                TAC* assign = add_tac(code->gen);
                assign->op = tac::assignment;
                assign->arg0.kind = arg::var;
                assign->arg0.var = state->temp;
                assign->arg1 = s;
                assign->node = second;
                array::push(code->gen->tac, assign);
            }
        }

        TAC* resolver = array::read(code->gen->tac, next_index);
        resolved->arg1.temporary = resolver;
        resolved->arg1.kind = arg::kind::temporary;
        resolver->jump_from = array::init<TAC*>();
        array::push(resolver->jump_from, resolved);

        // we've reached the end of an if/else ladder
        // so we can fill out the true body jumps
        if(second->kind != expr::conditional) {
            // we make a label TAC for the jumps, because we dont know yet
            // what the next instruction is actually going to be
            // this should be cleaned up eventually, ideally removing the need for making this at all
            TAC* end = add_tac(code->gen);
            end->op = tac::nop;
            end->jump_from = array::init<TAC*>();
            array::push(code->gen->tac, end);
            forI(state->truelist.count) {
                TAC* resolve = array::read(state->truelist, i);
                resolve->arg0 = end;
                array::push(end->jump_from, resolve);
            }
            end->node = second;
        }
    } else {
        // we're at the end, so we don't need a jump and we can backfill all previous jumps
        // and the hanging cond_jump
        TAC* end = add_tac(code->gen);
        end->op = tac::nop;
        end->jump_from = array::init<TAC*>();
        array::push(code->gen->tac, end);
        forI(state->truelist.count) {
            TAC* resolve = array::read(state->truelist, i);
            resolve->arg0 = end;            
            array::push(end->jump_from, resolve);
        }
        array::pop(state->falselist)->arg1 = end;
        end->node = first;
    }

    Arg out = {};
    if(state->temp) {
        out.kind = arg::var;
        out.var = state->temp;
    }
    return out;
}

Arg
expression(Code* code, Expr* e) {
    switch(e->kind) {
        case expr::varref: {
            return e->as<VarRef>()->var;
        } break;

        case expr::binary_plus: 
        case expr::binary_minus:
        case expr::binary_multiply:
        case expr::binary_division:
        case expr::binary_equal: 
        case expr::binary_not_equal: 
        case expr::binary_less_than:
        case expr::binary_less_than_or_equal:
        case expr::binary_greater_than:
        case expr::binary_greater_than_or_equal: {
            Arg lhs = expression(code, e->first_child<Expr>());
            Arg rhs = expression(code, e->last_child<Expr>());

            TAC* add = add_tac(code->gen); // this sucks 
            add->op = (e->kind == expr::binary_plus                  ? tac::addition :
                       e->kind == expr::binary_minus                 ? tac::subtraction :
                       e->kind == expr::binary_multiply              ? tac::multiplication :
                       e->kind == expr::binary_division              ? tac::division :
                       e->kind == expr::binary_equal                 ? tac::equal : 
                       e->kind == expr::binary_not_equal             ? tac::not_equal :
                       e->kind == expr::binary_less_than             ? tac::less_than :
                       e->kind == expr::binary_less_than_or_equal    ? tac::less_than_or_equal :
                       e->kind == expr::binary_greater_than          ? tac::greater_than :
                                                                       tac::greater_than_or_equal);
            add->arg0 = lhs;
            add->arg1 = rhs;

            array::push(code->gen->tac, add);
            add->node = e;
            return add;
        } break;

        case expr::cast: {
            return expression(code, e->first_child<Expr>());
        } break;
        case expr::literal: {
            TODO("handle literals other than unsigned ints");
            return e->start->u64_val;
        } break;
        case expr::binary_assignment: {
            Arg lhs = expression(code, e->first_child<Expr>());
            Arg rhs = expression(code, e->last_child<Expr>());

            TAC* tac = add_tac(code->gen);
            tac->op = tac::assignment;
            tac->arg0 = lhs;
            tac->arg1 = rhs;

            array::push(code->gen->tac, tac);

            tac->node = e;

            return tac;
        } break;
        case expr::unary_assignment: {
            // what this is being used for is handled by whatever called this
            return expression(code, e->last_child<Expr>());
        } break;
        case expr::call: {
            ScopedArray<Arg> returns = array::init<Arg>();

            auto ce = (Call*)e;
            for(auto n = ce->arguments->last_child<Expr>(); n; n = n->prev<Expr>()) {
                array::push(returns, expression(code, n));
            }

            forI(returns.count) {
                TAC* tac = add_tac(code->gen);
                tac->op = tac::param;
                Arg ret = array::read(returns, i);

                tac->arg0 = ret;

                array::push(code->gen->tac, tac);
            }

            TAC* tac = add_tac(code->gen);
            tac->op = tac::call;
            tac->arg0.kind = tac::arg::func;
            tac->arg0.func = ce->callee;

            array::push(code->gen->tac, tac);

            tac->node = e;

            return tac;
        } break;
        case expr::block: {
            block(code, (Block*)e);
            return array::read(code->gen->tac, -1);
        } break;
        case expr::conditional: {
            ConditionalState state = {};
            
            state.truelist = array::init<TAC*>();
            state.falselist = array::init<TAC*>();
            
            if(!e->first_child()->next<Expr>()
                 ->type->is_any<Void, Whatever>()) {
                state.temp = Var::create();
                state.temp->type = e->type;

                TAC* result = add_tac(code->gen);
                result->op = tac::temp;
                result->arg0.kind = tac::arg::var;
                result->arg0.var = state.temp;
                result->arg0.var->reg_offset = code->gen->registers.count;
                Register* r = array::push(code->gen->registers);
                r->idx = result->arg0.var->reg_offset;

                state.temp->label = 
                    VirtualLabel::create(
                            dstring::init("cond_temp(", code->gen->tac.count, ")"));

                result->node = e;

                array::push(code->gen->tac, result);
            }
            
            return conditional(code, e, &state);
        } break;

        case expr::loop: {
            u32 count = code->gen->tac.count;
            TAC* nop = add_tac(code->gen);
            nop->node = e;
            array::push(code->gen->tac, nop);
            array::push(code->gen->break_stacks, array::init<TAC*>());

            Arg arg = expression(code, e->first_child<Expr>());

            TAC* tac = add_tac(code->gen);
            tac->op = tac::jump;
            tac->arg0.kind = tac::arg::temporary;
            tac->arg0.temporary = array::read(code->gen->tac, count);
            tac->node = e;

            code->gen->resolve_breaks++;

            array::push(code->gen->tac, tac);

            return Arg();
        } break;

        case expr::break_: {
            TAC* jump = add_tac(code->gen);
            jump->op = tac::jump;
            jump->node = e;
            array::push(array::readref(code->gen->break_stacks, -1), jump);
            array::push(code->gen->tac, jump);

            return jump;
        } break;

        case expr::binary_or: {
            // logical or currently returns only 0 or 1
            // and is short circuiting
            // but later on I want to set it up to return the first true 
            // value so that we can do things like 
            // a := func() || other_func() || return;
            Var* v = Var::create();
            v->type = e->type;
            v->label = VirtualLabel::create(
                    dstring::init("or_temp(", code->gen->tac.count, ")"));
            
            TAC* temp = add_tac(code->gen);
            temp->op = tac::temp;
            temp->arg0.kind = arg::var;
            temp->arg0.var = v;
            temp->node = e;

            array::push(code->gen->tac, temp);

            auto jump_stack = array::init<TAC*>();

            auto left_stack = array::init<Expr*>();
            array::push(left_stack, e);
            auto left = e->first_child<Expr>();
            while(1) {
                if(left->is_not(expr::binary_or)) break;
                array::push(left_stack, left);
                left = left->first_child<Expr>();
            }

            Arg left_result;

            left = array::pop(left_stack);

            // get deepest lhs expression result 
            Arg lhs = expression(code, left->first_child<Expr>());

            TAC* ljump = add_tac(code->gen);
            ljump->op = tac::jump_not_zero;
            ljump->arg0 = lhs;

            array::push(jump_stack, ljump);
            array::push(code->gen->tac, ljump);

            // gather the rhs of each level
            while(1) {
                Arg rhs = expression(code, left->last_child<Expr>());
                TAC* rjump = add_tac(code->gen);
                rjump->op = tac::jump_not_zero;
                rjump->arg0 = rhs;
                rjump->node = e;

                array::push(code->gen->tac, rjump);
                array::push(jump_stack, rjump);

                if(!left_stack.count) break;
                left = array::pop(left_stack);
            }

            // TODO(sushi) this isn't necessary cause everything inits to 0
            // PROBABLY
            TAC* fail = add_tac(code->gen);
            fail->op = tac::assignment;
            fail->arg0.kind = tac::arg::var;
            fail->arg0.var = v;
            fail->arg1.kind = tac::arg::literal;
            fail->arg1.literal = 0;
            fail->node = e;

            array::push(code->gen->tac, fail);

            TAC* fail_jump = add_tac(code->gen);
            fail_jump->op = tac::jump;
            fail_jump->node = e;

            array::push(code->gen->tac, fail_jump);

            TAC* success = add_tac(code->gen);
            success->op = tac::assignment;
            success->arg0.kind = tac::arg::var;
            success->arg0.var = v;
            success->arg1.kind = tac::arg::literal;
            success->arg1.literal = 1;
            success->node = e;
            success->jump_from = array::init<TAC*>();

            array::push(code->gen->tac, success);

            // TODO(sushi) replace with a different backfilling system
            TAC* fin = add_tac(code->gen);
            fin->op = tac::nop;
            fin->node = e;
            fin->jump_from = array::init<TAC*>();
            array::push(fin->jump_from, fail_jump);

            array::push(code->gen->tac, fin);

            fail_jump->arg0.kind = tac::arg::temporary;
            fail_jump->arg0.temporary = fin;

            while(jump_stack.count) {
                TAC* resolved = array::pop(jump_stack);
                resolved->arg1.kind = tac::arg::temporary;
                resolved->arg1.temporary = success;
                resolved->node = e;
                array::push(success->jump_from, resolved);
            }
            
            return temp;
        } break;

        case expr::binary_and: {
            // logical and also returns 0 or 1
            // and is short circuiting
            // however I'm not sure if it would be possible
            // to choose a branch to return from to achieve 
            // an effect similar to what I want to do with logical or
            // maybe it returns the last branch that fails? especially if we 
            // want to handle an error
            // a := func?() && other_func?() && another_func?() && return;
            // if any func failed, a will have its value, otherwise the function
            // will have returned
            Arg lhs = expression(code, e->first_child<Expr>());
            Arg rhs = expression(code, e->last_child<Expr>());

            TAC* tac = add_tac(code->gen);
            tac->op = tac::logical_or;
            tac->arg0 = lhs;
            tac->arg1 = rhs;
            tac->node = e;
            array::push(code->gen->tac, tac);
            return tac;
        } break;
    }

    // an expression didn't return anything or this is a
    // completely unhandled expression kind
    Assert(0); 
    return {};
}

void
statement(Code* code, Stmt* s) {
    switch(s->kind) {
        case stmt::label: {
            auto l = s->first_child<Label>();
            switch(l->entity->kind) {
                case entity::var: {
                    auto v = l->entity->as<Var>();
                    Arg arg = expression(code, l->last_child<Expr>());

                    TAC* tac = add_tac(code->gen);
                    tac->op = tac::assignment;
                    tac->arg0.kind = tac::arg::var;
                    tac->arg0.var = v;

                    v->reg_offset = code->gen->registers.count;
#if BUILD_SLOW
                    Register* r = array::push(code->gen->registers);
                    r->v = v;
#else
                    array::push(code->gen->registers);
#endif

                    tac->arg1 = arg;
                    tac->node = s;

                    array::push(code->gen->tac, tac);
                } break;
                default: {
                    Assert(0); // unhandled label kind 
                } break;
            }
        } break;
        case stmt::expression: {
            if(s->flags.break_air_gen) {
                s->first_child()->flags.break_air_gen = true;
            }
            expression(code, s->first_child<Expr>());
        } break;
        case stmt::block_final: {
            if(s->first_child()) {
                // we are returning a value
                Arg arg = expression(code, s->first_child<Expr>());

                if(arg.kind == tac::arg::none && s->first_child()->is(expr::conditional)) {
                    // if there was no return and the child is a conditional, then this was mistakenly marked
                    // as a block final due to ending with 2 close braces
                    // TODO(sushi) fix the Parser mistakenly marking this as a block final
                } else {
                    TAC* ret = add_tac(code->gen);
                    ret->op = tac::block_value;
                    ret->arg0 = arg;
                    ret->node = s->first_child();
                    array::push(code->gen->tac, ret);
                }
            } else {
                // we are just returning from the function
                TAC* ret = add_tac(code->gen);
                ret->op = tac::op::ret;
                ret->node = s;
                array::push(code->gen->tac, ret);
            }
        } break;
    }
}

void
block(Code* code, Block* e) {
    TAC* tac = add_tac(code->gen);
    tac->op = tac::block_start;
    array::push(code->gen->tac, tac);
    for(auto n = e->first_child<Stmt>(); n; n = n->next<Stmt>()) {
        statement(code, n);
    }
    tac->node = e;

    if(array::read(code->gen->tac, -1)->op != tac::block_value) {
        tac = add_tac(code->gen);
        tac->op = tac::block_end;
        tac->node = e;
        array::push(code->gen->tac, tac);
    }
}

void
function(Code* code) {
    auto l  = code->parser->root->as<Label>();
    auto f  = l->entity->as<Function>();
    auto ft = f->type;

    block(code, l->last_child()->last_child<Block>());
} 

void
label(Code* code, Label* l) {
    switch(l->entity->kind) {
        case entity::func: {
            function(code);
        } break;
        case entity::var: {
            NotImplemented;
        } break;
    }
}

// void
// module(TNode* node) {
//     for(TNode* n = node->first_child; n; n = n->next) {
//         label((Label*)n);
//     }
// }

b32
start(Code* code) {
    switch(code->kind) {
        case code::source: {
            for(auto* n = code->first_child<Code>(); n; n = n->next<Code>()) {
                if(!generate(n)) return false;
            }
        } break;
        case code::function: {
            function(code);
        } break;
        default: {
            TODO(dstring::init("unhandled start case: ", code::strings[code->kind]));
            return false;
        } break;
    }
    return true;
}

b32
generate(Code* code) {
    if(!code->gen) code->gen = Gen::create(code);
    if(!start(code)) return false;

    util::println(code->identifier);
    u32 last_line_num = -1;
    forI(code->gen->tac.count) {
        TAC* tac = array::read(code->gen->tac, i);
        if(last_line_num == -1 || last_line_num != tac->node->start->l0) {
            // util::println(tac->node->first_line(true, true));
            last_line_num = tac->node->start->l0;
        }
        util::println(to_string(array::read(code->gen->tac, i)));
    }

    return true;
}

} // namespace tac

namespace air {

b32
function(Code* code) {
    // TODO(sushi) would it be worth caching this for large sequences?
    auto tacseq = code->gen->tac;

    // this probably shouldn't be how this is done, just 
    // doing it in a quick and dirty way for now 
    // it would probably be better to just make a new pair type of TAC and u32
    // and then cache the TAC sequence into it 
    // Keeps track of Register offsets for TAC that generates Registers 
    auto offset_map = map::init<TAC*, u32>();

    // this is pretty awful
    // these facilitate backfilling jumps that we come across in 
    auto false_jumps = array::init<Array<u32>>();
    auto true_jumps = array::init<Array<u32>>();

    struct Backpatch {
        u32 to_be_patched;
        TAC* patcher;
    };

    auto cond_backpatches = array::init<Backpatch>();
    auto jump_backpatches = array::init<Backpatch>();

    auto block_registers = array::init<Array<u32>>();

    forI(tacseq.count) {
        TAC* tac = array::read(tacseq, i);
        if(tac->node->flags.break_air_gen) DebugBreakpoint;

        forI(tac->jump_from.count) {
            TAC* fill = array::read(tac->jump_from, i);
            if(fill->bc_offset < code->gen->air.count) {
                BC* bc = array::readptr(code->gen->air, fill->bc_offset);
                switch(bc->instr) {
                    case op::jump_zero:
                    case op::jump_not_zero: {
                        bc->offset_b = code->gen->air.count - fill->bc_offset;
                    } break;

                    case op::jump: {
                        bc->offset_a = code->gen->air.count - fill->bc_offset;
                    } break;
                }
            }
        }

        // if(cond_backpatches.count) {
        //     Backpatch bp = array::read(cond_backpatches, -1);
        //     while(bp.patcher == tac) {
        //         array::pop(cond_backpatches);
        //         array::readref(code->gen->air, bp.to_be_patched).offset_b = code->gen->air.count - bp.to_be_patched;
        //         if(!cond_backpatches.count) break;
        //         bp = array::read(cond_backpatches, -1);
        //     }
        // }

        // if(jump_backpatches.count) {
        //     Backpatch bp = array::read(jump_backpatches, -1);
        //     while(bp.patcher == tac) {
        //         array::pop(jump_backpatches);
        //         array::readref(code->gen->air, bp.to_be_patched).offset_a = code->gen->air.count - bp.to_be_patched;
        //         if(!jump_backpatches.count) break;
        //         bp = array::read(jump_backpatches, -1);
        //     }
        // }
        switch(tac->op) {
            case tac::nop: {
                map::add(offset_map, tac, (u32)code->gen->air.count);
            } break;

            case tac::block_start: {
                auto br = array::init<u32>();
                array::push(block_registers, br);
            } break;

            case tac::block_end: {
                auto registers = array::pop(block_registers);
                forI(registers.count) {
                    BC* reset = array::push(code->gen->air);
                    reset->instr = op::copy;
                    reset->offset_a = array::read(registers, i);
                    reset->offset_b = 0;
                    reset->flags.right_is_const = true;
                    reset->node = tac->node;
                }
            } break;

            case tac::jump_zero: {
                BC* bc = array::push(code->gen->air);
                bc->node = tac->node;
                bc->instr = op::jump_zero;
                switch(tac->arg0.kind) {
                    case tac::arg::temporary: {
                        bc->offset_a = array::read(offset_map.values, map::find(offset_map, tac->arg0.temporary).index);
                    } break;
                    case tac::arg::literal: {
                        bc->flags.left_is_const = true;
                        bc->offset_a = tac->arg0.literal;
                    } break;
                    case tac::arg::var: {
                        bc->offset_a = tac->arg0.var->reg_offset; 
                    } break;
                }

                tac->bc_offset = code->gen->air.count-1;
            } break;

            case tac::jump_not_zero: {
                BC* bc = array::push(code->gen->air);
                bc->node = tac->node;
                bc->instr = op::jump_not_zero;
                switch(tac->arg0.kind) {
                    case tac::arg::temporary: {
                        bc->offset_a = array::read(offset_map.values, map::find(offset_map, tac->arg0.temporary).index);
                    } break;
                    case tac::arg::literal: {
                        bc->flags.left_is_const = true;
                        bc->offset_a = tac->arg0.literal;
                    } break;
                    case tac::arg::var: {
                        bc->offset_a = tac->arg0.var->reg_offset; 
                    } break;
                }

                tac->bc_offset = code->gen->air.count-1;
            } break;

            case tac::jump: {
                BC* bc = array::push(code->gen->air);
                bc->node = tac->node;
                bc->instr = op::jump;
                auto [idx, found] = map::find(offset_map, tac->arg0.temporary);

                if(found) {
                    bc->offset_a = array::read(offset_map.values, idx) - code->gen->air.count + 1;
                } 
                //     Backpatch* bp = array::insert(jump_backpatches, 0);
                //     bp->to_be_patched = code->gen->air.count-1;
                //     bp->patcher = tac->arg0.temporary;
                // }

                tac->bc_offset = code->gen->air.count-1;
            } break;

            case tac::temp: {
                map::add(offset_map, tac, (u32)code->gen->registers.count);
                Register* r = array::push(code->gen->registers);
                r->idx = code->gen->registers.count - 1;
                array::push(array::readref(block_registers, -1), r->idx);
            } break;

            case tac::addition:
            case tac::subtraction:
            case tac::multiplication:
            case tac::division: {
                u32 dest_offset = code->gen->registers.count;
                Register* r = array::push(code->gen->registers);
                r->idx = dest_offset;
                array::push(array::readref(block_registers, -1), r->idx);

                BC* bc0 = array::push(code->gen->air);
                bc0->node = tac->node;
                bc0->instr = op::add;
                bc0->offset_a = dest_offset;

                switch(tac->arg0.kind) {
                    case tac::arg::temporary: {
                        bc0->offset_b = array::read(offset_map.values, map::find(offset_map, tac->arg0.temporary).index);
                    } break;
                    case tac::arg::literal: {
                        bc0->flags.right_is_const = true;
                        bc0->offset_b = tac->arg0.literal;
                    } break;
                    case tac::arg::var: {
                        bc0->offset_b = tac->arg0.var->reg_offset; 
                    } break;
                }

                BC* bc1 = array::push(code->gen->air);
                bc1->node = tac->node;
                bc1->instr = 
                    (tac->op == tac::addition ?       op::add :
                     tac->op == tac::subtraction ?    op::sub :
                     tac->op == tac::multiplication ? op::mul :
                                                      op::div);
                bc1->offset_a = dest_offset;

                switch(tac->arg1.kind) {
                    case tac::arg::temporary: {
                        bc1->offset_b = array::read(offset_map.values, map::find(offset_map, tac->arg1.temporary).index);
                    } break;
                    case tac::arg::literal: {
                        bc1->flags.right_is_const = true;
                        bc1->offset_b = tac->arg1.literal;
                    } break;
                    case tac::arg::var: {
                        bc1->offset_b = tac->arg1.var->reg_offset; 
                    } break;
                }

                map::add(offset_map, tac, dest_offset);
            } break;
            
            case tac::equal:
            case tac::not_equal:
            case tac::less_than:
            case tac::less_than_or_equal:
            case tac::greater_than:
            case tac::greater_than_or_equal: {
                u32 dest_offset = code->gen->registers.count;
                Register* r = array::push(code->gen->registers);
                r->idx = dest_offset;
                array::push(array::readref(block_registers, -1), r->idx);

                BC* bc0 = array::push(code->gen->air);
                bc0->node = tac->node;
                bc0->instr = op::add;
                bc0->offset_a = dest_offset;

                switch(tac->arg0.kind) {
                    case tac::arg::temporary: {
                        bc0->offset_b = array::read(offset_map.values, map::find(offset_map, tac->arg0.temporary).index);
                    } break;
                    case tac::arg::literal: {
                        bc0->flags.right_is_const = true;
                        bc0->offset_b = tac->arg0.literal;
                    } break;
                    case tac::arg::var: {
                        bc0->offset_b = tac->arg0.var->reg_offset; 
                    } break;
                }

                BC* bc1 = array::push(code->gen->air);
                bc1->node = tac->node;
                bc1->instr = 
                    (tac->op == tac::equal                ? op::eq  :
                     tac->op == tac::not_equal            ? op::neq :
                     tac->op == tac::less_than            ? op::lt  :
                     tac->op == tac::less_than_or_equal   ? op::le  :
                     tac->op == tac::greater_than         ? op::gt  :
                                                            op::ge);
                bc1->offset_a = dest_offset;    

                switch(tac->arg1.kind) {
                    case tac::arg::temporary: {
                        bc1->offset_b = array::read(offset_map.values, map::find(offset_map, tac->arg1.temporary).index);
                    } break;
                    case tac::arg::literal: {
                        bc1->flags.right_is_const = true;
                        bc1->offset_b = tac->arg1.literal;
                    } break;
                    case tac::arg::var: {
                        bc1->offset_b = tac->arg1.var->reg_offset; 
                    } break;
                }

                map::add(offset_map, tac, dest_offset);
            } break;
            
            case tac::logical_or: {

            } break;

            case tac::assignment: {
                BC* bc = array::push(code->gen->air);
                bc->node = tac->node;
                bc->instr = op::copy;

                switch(tac->arg0.kind) {
                    case tac::arg::temporary: {
                        bc->offset_a = array::read(offset_map.values, map::find(offset_map, tac->arg0.temporary).index);
                    } break;
                    case tac::arg::var: {
                        bc->offset_a = tac->arg0.var->reg_offset;
                    } break;
                }

                switch(tac->arg1.kind) {
                    case tac::arg::temporary: {
                        bc->offset_b = array::read(offset_map.values, map::find(offset_map, tac->arg1.temporary).index);
                    } break;
                    case tac::arg::var: {
                        bc->offset_b = tac->arg1.var->reg_offset;
                    } break;
                    case tac::arg::literal: {
                        bc->flags.right_is_const = true;
                        bc->offset_b = tac->arg1.literal;
                    } break;
                }
            } break;

            case tac::block_value: {
                u32 offset = (u32)code->gen->registers.count;
                map::add(offset_map, tac, offset);
                Register* r = array::push(code->gen->registers);
                r->idx = offset;
                array::push(array::readref(block_registers, -1), r->idx);

                BC* bc = array::push(code->gen->air);
                bc->node= tac->node;
                bc->instr = op::copy;

                bc->offset_a = offset;
                switch(tac->arg0.kind) {
                    case tac::arg::temporary: {
                        bc->offset_b = array::read(offset_map.values, map::find(offset_map, tac->arg0.temporary).index);
                    } break;
                    case tac::arg::var: {
                        bc->offset_b = tac->arg0.var->reg_offset;
                    } break;
                    case tac::arg::literal: {
                        bc->flags.right_is_const = true;
                        bc->offset_b = tac->arg0.literal;
                    } break;
                }
            } break;
        }
    }
    return true;
}

b32
start(Code* code) {
    switch(code->kind) {
        case code::source: {
            for(auto* n = code->first_child<Code>(); n; n = n->next<Code>()) {
                if(!generate(n)) return false;
            }
        } break;
        case code::function: {
            function(code);
        } break;
        default: {
            TODO(dstring::init("unhandled start case: ", code::strings[code->kind]));
            return false;
        } break;
    }
    return true;
}

b32
generate(Code* code) {
    if(!start(code)) return false;

    util::println(code->identifier);
    u32 last_line_num = -1;
    forI(code->gen->air.count) {
        BC* bc = array::readptr(code->gen->air, i);
        if(last_line_num == -1 || last_line_num != bc->node->start->l0) {
            util::println(bc->node->first_line(true, true));
            last_line_num = bc->node->start->l0;
        }
        util::println(to_string(bc, code));
    }

    return true;
}
} // namespace air

Gen* Gen::
create(Code* code) {
    Gen* out = pool::add(compiler::instance.storage.gens);
    out->tac_pool = pool::init<TAC>(128);
    out->tac = array::init<TAC*>();
    out->registers = array::init<Register>();
    out->air = array::init<BC>();
    code->gen = out;
    return out;
}

void
to_string(DString& current, tac::Arg arg) {
    switch(arg.kind) {
        case tac::arg::literal: {
            dstring::append(current, arg.literal);
        } break;
        case tac::arg::var: {
            dstring::append(current, arg.var->name());
        } break;
        case tac::arg::func: {
            dstring::append(current, arg.func->name());
        } break;
        case tac::arg::temporary: {
            dstring::append(current, "(", arg.temporary->id, ")");
        } break;
    }
}

void
to_string(DString& current, TAC* tac) {
    dstring::append(current, "(", tac->id, ") ~ ");
    switch(tac->op) {
        case tac::nop: {
            dstring::append(current, "nop");
        } break;
        case tac::temp: {
            dstring::append(current, "temp");
        } break;
        case tac::stack_push: {
            dstring::append(current, "stack_push ", tac->arg0);
        } break;
        case tac::stack_pop: {
            dstring::append(current, "stack_pop ", tac->arg0);
        } break;
        case tac::addition: {
            dstring::append(current, tac->arg0, " + ", tac->arg1);
        } break;
        case tac::subtraction: {
            dstring::append(current, tac->arg0, " - ", tac->arg1);
        } break;
        case tac::multiplication: {
            dstring::append(current, tac->arg0, " * ", tac->arg1);
        } break;
        case tac::division: {
            dstring::append(current, tac->arg0, " / ", tac->arg1);
        } break;
        case tac::assignment: {
            dstring::append(current, tac->arg0, " = ", tac->arg1);
        } break;
        case tac::equal: {
            dstring::append(current, tac->arg0, " == ", tac->arg1);
        } break;
        case tac::not_equal: {
            dstring::append(current, tac->arg0, " != ", tac->arg1);
        } break;
        case tac::less_than: {
            dstring::append(current, tac->arg0, " < ", tac->arg1);
        } break;
        case tac::less_than_or_equal: {
            dstring::append(current, tac->arg0, " <= ", tac->arg1);
        } break;
        case tac::greater_than: {
            dstring::append(current, tac->arg0, " > ", tac->arg1);
        } break;
        case tac::greater_than_or_equal: {
            dstring::append(current, tac->arg0, " >= ", tac->arg1);
        } break;
        case tac::param: {
            dstring::append(current, "param ", tac->arg0);
        } break;
        case tac::call: {
            dstring::append(current, "call ", tac->arg0);
        } break;
        case tac::block_start: {
            dstring::append(current, "block_start");
        } break;
        case tac::block_end: {
            dstring::append(current, "block_end");
        } break;
        case tac::block_value: {
            dstring::append(current, "block_value ", tac->arg0);
        } break;
        case tac::ret: {
            dstring::append(current, "return ");
            if(tac->arg0.kind) {
                dstring::append(current, tac->arg0);
            }
        } break;
        case tac::jump: {
            dstring::append(current, "jump ");
            if(tac->arg0.kind) {
                dstring::append(current, tac->arg0);
            } else {
                dstring::append(current, "...");
            }
        } break;
        case tac::jump_zero: {
            dstring::append(current, "jump_zero ", tac->arg0, " ");
            if(tac->arg1.kind) {
                dstring::append(current, tac->arg1);
            } else {
                dstring::append(current, "...");
            }
        } break;
        case tac::jump_not_zero: {
            dstring::append(current, "jump_not_zero ", tac->arg0, " ");
            if(tac->arg1.kind) {
                dstring::append(current, tac->arg1);
            } else {
                dstring::append(current, "...");
            }
        } break;
       
    }
}

void
to_string(DString& current, BC* bc, Code* c) {
    switch(bc->instr) {
        case air::op::add: {
            dstring::append(current, "add ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;

        case air::op::sub: {
            dstring::append(current, "sub ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;

        case air::op::mul: {
            dstring::append(current, "mul ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;

        case air::op::div: {
            dstring::append(current, "div ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;

        case air::op::copy: {
            dstring::append(current, "copy ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;
        case air::op::jump: {
            dstring::append(current, "jmp ", bc->offset_a, " ");
        } break;
        case air::op::jump_zero: {
            dstring::append(current, "jz ");
            if(bc->flags.left_is_const) {
                dstring::append(current, bc->offset_a, " ");
            }else{
                dstring::append(current, array::read(c->gen->registers, bc->offset_a), " ");
            }
            dstring::append(current, bc->offset_b);
        } break;
        case air::op::jump_not_zero: {
            dstring::append(current, "jnz ");
            if(bc->flags.left_is_const) {
                dstring::append(current, bc->offset_a, " ");
            }else{
                dstring::append(current, array::read(c->gen->registers, bc->offset_a), " ");
            }
            dstring::append(current, bc->offset_b);
        } break;
        case air::op::eq:{
            dstring::append(current, "eq ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
            
        } break;    
        case air::op::neq:{
            dstring::append(current, "neq ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;
        case air::op::lt:{
            dstring::append(current, "lt ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;
        case air::op::gt:{
            dstring::append(current, "gt ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;
        case air::op::le:{
            dstring::append(current, "le ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;
        case air::op::ge:{
            dstring::append(current, "ge ", array::read(c->gen->registers, bc->offset_a), " ");
            if(bc->flags.right_is_const) {
                dstring::append(current, bc->offset_b);
            } else {
                dstring::append(current, array::read(c->gen->registers, bc->offset_b));
            }
        } break;
    }
}

void
to_string(DString& current, Register r) {
#if BUILD_SLOW
    if(r.v) {
        dstring::append(current, "r", r.idx, " (", r.v->name(), ")");
    } else {
        dstring::append(current, "r", r.idx);
    }
#else
    dstring::append(current, "r", index);
#endif
}

} // namespace amu