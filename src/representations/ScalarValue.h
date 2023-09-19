/*

    Internal representation of a singular number in amu. This represents signed, unsigned, and floating 
    point numbers and has an interface for converting between them.

    If amu ever comes to natively support complex numbers, those should probably be implemented as part
    of this structure.

    The purpose of this structure is to provide a uniform representation of all of the possible scalars that
    may appear in amu. This isn't meant to be allocated anywhere, just used on certain structs, such as
    ScalarLiteral and Arg. The type of Scalar being represented is determined by the Scalar type it points
    to.

    The motivation for this comes from finding myself writing out unions of different scalar values and enums to 
    determine what kind of scalar it is in several places and having to convert between them everytime they mixed. 
    This should allow me to just straight copy between the different usages when necessary.


    TODO

    
    Unfortunately doing this this way causes some duplication between Scalar and ScalarValue, particularly
    is_signed and is_float. It also causes there to be two kinds to keep track of with ScalarLiteral. I wanted
    to try and make this inherit Scalar and then have ScalarLiteral inherit ScalarValue, but this runs into the
    diamond problem, and that's too OOP for me to want to deal with. 

    Ideally this can be cleaned up some other way, but this should do for now. 
*/

#ifndef AMU_SCALARVALUE_H
#define AMU_SCALARVALUE_H

#include "Type.h"

namespace amu {

struct ScalarValue : public Base {
    scalar::kind kind;
    union {
        u64 _u64;
        u32 _u32;
        u16 _u16;
        u8  _u8;

        s64 _s64;
        s32 _s32;
        s16 _s16;
        s8  _s8;

        f64 _f64;
        f32 _f32;
    };

    DString*
    display();

    DString*
    dump();

    b32
    is_signed();

    b32
    is_float();

    b32
    is_negative();

    u64
    size();

    // casts this ScalarValue IN PLACE.
    void
    cast_to(scalar::kind k);

    ScalarValue() : _u64(0), kind(scalar::unsigned64), Base(base::scalar_value) {}

    ScalarValue(u64 x) : _u64(x), kind(scalar::unsigned64), Base(base::scalar_value) {}
    ScalarValue(u32 x) : _u32(x), kind(scalar::unsigned32), Base(base::scalar_value) {}
    ScalarValue(u16 x) : _u16(x), kind(scalar::unsigned16), Base(base::scalar_value) {}
    ScalarValue(u8  x) : _u8 (x), kind(scalar::unsigned8), Base(base::scalar_value) {}
    ScalarValue(s64 x) : _s64(x), kind(scalar::signed64), Base(base::scalar_value) {}
    ScalarValue(s32 x) : _s32(x), kind(scalar::signed32), Base(base::scalar_value) {}
    ScalarValue(s16 x) : _s16(x), kind(scalar::signed16), Base(base::scalar_value) {}
    ScalarValue(s8  x) : _s8 (x), kind(scalar::signed8), Base(base::scalar_value) {}
    ScalarValue(f64 x) : _f64(x), kind(scalar::float64), Base(base::scalar_value) {}
    ScalarValue(f32 x) : _f32(x), kind(scalar::float32), Base(base::scalar_value) {}
};



} // namespace amu 

#endif // AMU_SCALARVALUE_H