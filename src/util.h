/*
    Various helpful utilities or definitions for use throughout amu and other various things
	that don't really have a place anywhere else.

	This file is likely to be included in most places, so be careful about editing it.
*/

#ifndef AMU_UTIL_H
#define AMU_UTIL_H

#include "basic/Memory.h"

namespace amu {
namespace util {

template<typename... T> FORCE_INLINE b32
any(const T&... args) {
	return (args || ...);
}

template<typename... T> FORCE_INLINE b32
all(const T&... args) {
	return (args && ...);
}

template<typename A, typename... B> FORCE_INLINE b32
any_match(const A& v, B... args) {
	return ((v == args) || ...);
}

template<typename A, typename... B> FORCE_INLINE b32
all_match(const A& v, B... args) {
	return ((v == args) && ...);
}

constexpr s64 
constexpr_strlen(const char* s) {
    s64 i = 0;
    while(s[i]) {
        i++;
    }
    return i;
}

constexpr upt
round_up_to(s64 value, s64 multiple) {
	return  (((upt)((value) + (((upt)(multiple))-1)) / (upt)(multiple)) * (upt)(multiple));
}
							
// specializable generic hash functions
template<typename T> u64
hash(const T& x) {
	u32 seed = 2166136261;
	size_t data_size = sizeof(T);
	const u8* data = (const u8*)&x;
	while (data_size-- != 0) {
		seed ^= *data++;
		seed *= 16777619;
	}
	return seed;
}

template<typename T> u64
hash(T* x) {
	u32 seed = 2166136261;
	size_t data_size = sizeof(T);
	const u8* data = (const u8*)x;
	while (data_size-- != 0) {
		seed ^= *data++;
		seed *= 16777619;
	}
	return seed;
}

template<typename A, typename B> FORCE_INLINE A
Min(A x, B y) { return (x<y?x:y); }

template<typename A, typename B> FORCE_INLINE A
Max(A x, B y) { return (x>y?x:y); }

// experimental type for creating scoped types when needed
// this is to avoid making deconstructors, since you can't explicitly control
// those. This isn't meant to be used in place, as that would be very ugly,
// it should be typedef'd instead, for example: 
//   typedef scoped<RString, [](RString* x) { ostring::dereference(*x); } ScopedRString;
template<typename T, void (*cleanup)(T*)>
struct scoped : public T {
	scoped(const T& in) {memory.copy(this, (void*)&in, sizeof(T));}
	~scoped() {cleanup((T*)this);}
};

}



} // namespace amu

#endif // AMU_UTIL_H
