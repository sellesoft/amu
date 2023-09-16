namespace amu {

VM* VM::
create(Code* entry) {
    VM* out = pool::add(compiler::instance.storage.vm);
    // arbitrary amount of stack to allocate
    // this needs to be a compiler option later 
    out->stack = (u8*)memory::allocate(Megabytes(8));
    entry->machine = out;
    
    if(entry->is(code::function)) {
        out->frame = entry->parser->root->as<Label>()->entity->as<Function>()->frame;
    } else if(entry->is(code::expression)) {
        // we create a frame for this expr
        out->frame.identifier = DString::create("ExprFrame<", (void*)entry->parser->root, ">");
    }
    out->frame.ip = entry->air_gen->seq.data;
    out->frame.fp = out->stack;
    out->sp = out->frame.fp;

    return out;
}

void VM::
destroy() {
    memory::free(stack);
    pool::remove(compiler::instance.storage.vm, this);
}

void VM::
run() {

    auto start = util::stopwatch::start();

    // return;

    #define sized_op(op, sz, dst, src)                \
    switch(sz) {                                      \
        case byte: *(dst) op (u8)(src); break;        \
        case word: *(u16*)(dst) op (u16)(src); break; \
        case dble: *(u32*)(dst) op (u32)(src); break; \
        case quad: *(u64*)(dst) op (u64)(src); break; \
    }

    #define sized_op_flt(op, sz, dst, src)            \
    switch(sz) {                                      \
        case dble: *(f32*)(dst) op (f32)(src); break; \
        case quad: *(f64*)(dst) op (f64)(src); break; \
    }

    #define sized_op_flt_addr(op, sz, dst, src)            \
    switch(sz) {                                      \
        case dble: *(f32*)(dst) op (f32)(*(f32*)src); break; \
        case quad: *(f64*)(dst) op (f64)(*(f64*)src); break; \
    }

    #define sized_op_assign(op, sz, dst, src)                        \
    switch(sz) {                                                     \
        case byte: *(dst) = *(dst) op (u8)(src); break;              \
        case word: *(u16*)(dst) = *(u16*)(dst) op (u16)(src); break; \
        case dble: *(u32*)(dst) = *(u32*)(dst) op (u32)(src); break; \
        case quad: *(u64*)(dst) = *(u64*)(dst) op (u64)(src); break; \
    }

    #define sized_op_assign_flt(op, sz, dst, src)                    \
    switch(sz) {                                                     \
        case dble: *(f32*)(dst) = *(f32*)(dst) op (f32)(src); break; \
        case quad: *(f64*)(dst) = *(f64*)(dst) op (f64)(src); break; \
    }

    #define sized_op_assign_flt_addr(op, sz, dst, src)               \
    switch(sz) {                                                     \
        case dble: *(f32*)(dst) = *(f32*)(dst) op (f32)(*(f32*)src); break; \
        case quad: *(f64*)(dst) = *(f64*)(dst) op (f64)(*(f64*)src); break; \
    }

    b32 finished = 0;
    while(!finished) { 
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        BC* instr = frame.ip;
        util::println(DString::create("----------------------- ", frame.ip, " of ", frame.identifier, " with sp ", sp - frame.fp));
        util::println(instr->node->first_line());
        util::println(ScopedDeref(to_string(*frame.ip)).x);
        switch(instr->instr) {
            case air::op::push: {
                u8* dst = sp;
                if(frame.ip->flags.left_is_const) {
                    if(instr->flags.float_op) {
                        sized_op_flt(=, instr->w, dst, instr->lhs);
                    } else {
                        sized_op(=, instr->w, dst, instr->lhs);
                    }
                    sp += instr->rhs;
                } else {
                    u8* src = frame.fp + frame.ip->lhs;
                    if(instr->flags.left_is_ptr) {
                        if(instr->flags.deref_left) {
                            src = frame.fp + *(u64*)instr->lhs;
                        } else {
                            src = (u8*)instr->lhs;
                        }
                    } else {
                        if(instr->flags.deref_left) {
                            src = frame.fp + *(u64*)(frame.fp + frame.ip->lhs);
                        } else {
                            src = frame.fp + frame.ip->lhs;
                        }
                    }
                    memory::copy(dst, src, instr->rhs);
                    sp += instr->rhs;
                }
            } break;

            case air::op::pushn: sp += frame.ip->lhs; break;
            case air::op::popn:  sp -= frame.ip->lhs; break;

            case air::op::copy: {
                u8* dst = 0;
                if(instr->flags.left_is_ptr) {
                    dst = (u8*)instr->copy.dst;
                } else {
                    dst = frame.fp + instr->copy.dst;
                }
                if(instr->flags.right_is_const) {
                    switch(instr->copy.literal.kind) {
                        case scalar::float32: *(f32*)dst = instr->copy.literal._f32; break;
                        case scalar::float64: *(f64*)dst = instr->copy.literal._f64; break;
                        case scalar::signed8: 
                        case scalar::unsigned8: *(u8*)dst = instr->copy.literal._u8; break;
                        case scalar::signed16: 
                        case scalar::unsigned16: *(u16*)dst = instr->copy.literal._u16; break;
                        case scalar::signed32: 
                        case scalar::unsigned32: *(u32*)dst = instr->copy.literal._u32; break;
                        case scalar::signed64: 
                        case scalar::unsigned64: *(u64*)dst = instr->copy.literal._u64; break;
                    }
                } else {
                    u8* src = frame.fp + instr->copy.src;
                    memory::copy(dst, src, instr->copy.size);
                }
            } break;

            case air::op::add: {
               u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(+=, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(+=, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_flt_addr(+=, instr->w, dst, src);
                    } else {
                        sized_op(+=, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::sub: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(-=, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(-=, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_flt_addr(-=, instr->w, dst, src);
                    } else {
                        sized_op(-=, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::mul: {
                u8* dst = 0;
                if(instr->flags.left_is_ptr) {
                    dst = (u8*)instr->lhs;
                } else {
                    dst = frame.fp + instr->lhs;
                }
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(*=, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(*=, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = 0;
                    if(instr->flags.right_is_ptr) {
                        src = (u8*)instr->rhs;
                    } else {
                        src = frame.fp + instr->rhs;
                    }
                    if(frame.ip->flags.float_op) {
                        sized_op_flt_addr(*=, instr->w, dst, src);
                    } else {
                        sized_op(*=, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::div: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(/=, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(/=, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_flt_addr(/=, instr->w, dst, src);
                    } else {
                        sized_op(/=, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::jump_zero: {
                if(frame.ip->flags.left_is_const) {
                    if(!frame.ip->lhs) frame.ip += frame.ip->rhs - 1;
                } else {
                    u8* src = frame.fp + frame.ip->lhs;
                    b32 cond = 0;
                    switch(instr->w) {
                        case byte: cond = cond == *src; break;
                        case word: cond = cond == *(u16*)src; break;
                        case dble: cond = cond == *(u32*)src; break;
                        case quad: cond = cond == *(u64*)src; break;
                    }
                    if(cond) frame.ip += frame.ip->rhs - 1;
                }
            } break;

            case air::op::jump_not_zero: {
                if(frame.ip->flags.left_is_const) {
                    if(frame.ip->lhs) frame.ip += frame.ip->rhs - 1;
                } else {
                    u8* src = frame.fp + frame.ip->lhs;
                    b32 cond = 1;
                    switch(instr->w) {
                        case byte: cond = cond == *src; break;
                        case word: cond = cond == *(u16*)src; break;
                        case dble: cond = cond == *(u32*)src; break;
                        case quad: cond = cond == *(u64*)src; break;
                    }
                    if(cond) frame.ip += frame.ip->rhs - 1;
                }
            } break;

            case air::op::jump: {
                frame.ip += frame.ip->lhs - 1;
            } break;

            // TODO(sushi) this sucks do it better later 
            case air::op::eq: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(==, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(==, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_assign_flt_addr(==, instr->w, dst, src);
                    } else {
                        sized_op(==, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::neq: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(!=, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(!=, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_assign_flt_addr(!=, instr->w, dst, src);
                    } else {
                        sized_op(!=, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::lt: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(<, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(<, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_assign_flt_addr(<, instr->w, dst, src);
                    } else {
                        sized_op(<, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::gt: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(>, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(>, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_assign_flt_addr(>, instr->w, dst, src);
                    } else {
                        sized_op(>, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::le: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(<=, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(<=, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_assign_flt_addr(<=, instr->w, dst, src);
                    } else {
                        sized_op(<=, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::ge: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(frame.ip->flags.right_is_const) {
                    if(frame.ip->flags.float_op) {
                        sized_op_flt(>=, instr->w, dst, instr->rhs_f);
                    } else {
                        sized_op(>=, instr->w, dst, frame.ip->rhs);
                    }
                } else {
                    u8* src = frame.fp + frame.ip->rhs;
                    if(frame.ip->flags.float_op) {
                        sized_op_assign_flt_addr(>=, instr->w, dst, src);
                    } else {
                        sized_op(>=, instr->w, dst, *src);
                    }
                }
            } break;

            case air::op::call: {
                frames.push(frame);
                u8* next_fp = sp - frame.ip->n_params;
                frame = frame.ip->f->frame;
                frame.fp = next_fp;
                sp = frame.fp;
                continue; // we don't want to increment the ip
            } break;

            case air::op::ret: {
                if(!frames.count){
                    finished = true;
                    break;
                }
                Frame last = frame;
                sp = frame.fp + frame.ip->lhs; 
                frame = frames.pop();
                util::println(to_string(*frame.ip));
            } break;

            case air::op::resz: {
                u8* dst = frame.fp + frame.ip->lhs;
                if(instr->flags.float_op) {
                    if(instr->w == width::quad) {
                        *(f64*)(dst) = *(f32*)dst; 
                    } else {
                        *(f32*)(dst) = *(f64*)dst; 
                    }
                } else {
                    TODO("integer resizing");
                }
            } break;
        }

        frame.ip += 1;

        print_frame_vars();
    } 

    util::println(util::format_time(util::stopwatch::peek(start)));

} 

void VM::
print_stack() {
    DString* out = DString::create();

    auto my_frames = Array<Frame*>::create();
    forI(frames.count)
        my_frames.push(frames.readptr(i));
    my_frames.push(&frame);

    u32 frame_idx = 0;

    u8* p = stack;
    while(p < sp) {
        out->append("r", p-stack, " ", *p);
        if(frame_idx < my_frames.count && my_frames.read(frame_idx)->fp == p) {    
            out->append(" <-- fp of ", my_frames.read(frame_idx)->identifier);
            frame_idx++;
        }
        out->append("\n");
        p++;
    }
    util::println(out);
    out->deref();
}

void VM::
print_frame_vars() {
    DString* out = DString::create();

    forI(frame.locals.count) {
        Var* v = frame.locals.read(i);
        auto val = v->type->print_from_address(frame.fp + v->stack_offset);
        val->indent(2);
        out->append(ScopedDeref(v->display()).x, "(", v->stack_offset, "): \n", ScopedDeref(val).x, "\n");
    }

    util::println(out->fin);
    out->deref();
}

} // namespace amu
