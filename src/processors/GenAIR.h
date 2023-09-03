/*

    AIR generator. See AIR.h for information on what AIR is.

    This takes Code that has generated TAC, optionally been optimized, and generates a sequence of
    AIR representing it. 

*/

#ifndef AMU_GENAIR_H
#define AMU_GENAIR_H

namespace amu {

struct GenAIR {
    Code* code;

    Array<BC> seq;

    u32 stack_offset;

    Array<u64> scoped_temps;

    static GenAIR*
    create(Code* code);

    void
    generate();

private:
    void start();
    void body();

    // creates room on the stack for data of some Type
    // then returns the beginning of that data
    u64
    new_reg(Type* t);

    void
    push_temp(TAC* tac);

    void
    clean_temps();

    void
    push_scope();

    void
    pop_scope();
};

} // namespace amu 

#endif // AMU_GENAIR_H