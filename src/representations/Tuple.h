/*

    Representation of any contiguous grouping of things in amu.
    For example, 
    
    label groups
        a,b,c := func();
        ~~~~~Tuple

    function parameters
        func :: (a:u32, b:u32) -> u32;
                 ~~~~~~~~~~~~Tuple

    function call arguments
        func(1, 2);
             ~~~~Tuple

    function multi return
        func :: () -> u32, s32, u8[..];
                      ~~~~~~~~~~~~~~~~Tuple

    amu's builtin Tuple type
        thing : (u32, b32, u8[..]);
                 ~~~~~~~~~~~~~~~~Tuple

    The elements of a Tuple are its children in the AST.
    
*/

#ifndef AMU_TUPLE_H
#define AMU_TUPLE_H

#include "basic/Node.h"
#include "Label.h"

namespace amu {

struct TupleType;

namespace tuple{
// @genstrings(data/tuple_strings.generated)
enum kind : u32 {
    unknown,
    label_group,
    parameters,
    arguments,
    multireturn,
    builtin,
};

#include "data/tuple_strings.generated"

} // namespace tuple

struct Tuple : public ASTNode {
    tuple::kind kind;
    // if this tuple applies names to its elements, they are stored here
    LabelTable table;
    // when this is a valued Tuple, this points to the underlying type
    TupleType* type;


    // ~~~~~~ interface ~~~~~~~


    static Tuple*
    create();

    void
    destroy();

    String
    name();

    DString
    debug_str();

    Tuple() : ASTNode(ast::tuple) {}
};

template<> inline b32 ASTNode::
is<Tuple>() { return kind == ast::tuple; }

template<> inline b32 ASTNode::
next_is<Tuple>() { return next() && next()->is<Tuple>(); }

void
to_string(DString& start, Tuple* t);

} // namespace amu

#endif // AMU_TUPLE_H