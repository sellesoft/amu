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

#include "basic/Node.h"

namespace amu {

struct Tuple {
    TNode node;
    Type type;
};

namespace tuple {

enum TupleType : u32 {
    label_group,
    parameters,
    arguments,
    multireturn,
    builtin,
};

} // namespace tuple

} // namespace amu