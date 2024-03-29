/*

    TAC generator. See TAC.h for information on what TAC is.

    This takes Code that has been parsed and semantically validated and generates a sequence
    of TAC representing it.

    TODO(sushi) currently TAC keeps track of where its temporaries should be placed on the 
                stack, but this completely undermines the ability of TACs to be freely
                movable. 
                An issue arises when we don't, though. We need to somehow communicate
                to AIR that a function that takes parameters 

*/

#ifndef AMU_GENTAC_H
#define AMU_GENTAC_H

namespace amu {

struct GenTAC {
    Code* code;

    // TAC are stored in pools so that they do not move and the actual sequence of 
    // TAC is stored in the 'seq' array. This is done so that in optimization, or possibly
    // user manipulations, the TAC seq can be reordered freely without needing to worry
    // about repointing TAC around the place.
    u64 count;
    Pool<TAC> pool;
    Array<TAC*> seq;

    // when we come across a 'loop', it creates a TAC for its start and end, pushes them
    // into these two arrays, then generates TAC for the following expression. This allows
    // breaks/continues/other control flow to easily discern what TAC they need to jump to  
    Array<TAC*> loop_start_stack;
    Array<TAC*> loop_end_stack;
    
    // scoped arrays of TAC that generated temporaries. We need to pop
    // temporaries so that the stack doesn't keep growing 
    Array<Array<TAC*>> temps;

    // array of local variables in the order that they are found
    // for AIR to position on the stack later 
    Array<Var*> locals;
    Array<Var*> params; 

    Array<Var*> varrefs;

    u64 register_offset;

	Future<void> fut;


	// ~~~~ interface ~~~~


    static GenTAC*
    create(Code* code);

    void
    destroy();

    void
    generate();

private:
    // internal tac generating functions
    void start();
    void label(Label* l);
    void function();
    void block(Block* e);
    void statement(Stmt* s);
    Arg  expression(Expr* e);

    // creates a TAC in the pool 
    // but doesn't push it to the sequence
    TAC*
    make();

    void
    place(TAC* t);

    // creates a TAC in the pool and pushes it into the 
    // sequence immediately
    TAC*
    make_and_place();

    // connects jump links between 'from' and 'to' 
    void
    link_jump_to(TAC* to, TAC* from);

    // create a new temp location on the stack with the size of Type
    void
    new_temp(TAC* tac, Type* t);

};

} // namespace amu 

#endif // AMU_GENTAC_H
