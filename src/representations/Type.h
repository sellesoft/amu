/*

    Structures for representing various kind of type in amu and an interface for interacting with them.

*/

#ifndef AMU_TYPE_H
#define AMU_TYPE_H

// #include "Token.h"

#include "Entity.h"
#include "Code.h"
#include "Token.h"

namespace amu {

struct Function;
struct Member;
struct Expr;
struct Tuple;

// base structure of all types, though this is not meant to be created directly
struct Type : public Entity {
	enum class Kind {
		Null,
		Void,
		Control,
		Whatever,
		Scalar,
		Structured,
		Pointer,
		Function,
		Module,
		Range,
		Tuple,
		Meta,
	};

    Kind kind;

    // pointer to the expression that defines this type 
    Expr* def;

    // set of traits applied to this Type
    Array<Trait*> traits;

    // Function objects that define this type as its first parameter
    Array<Function*> methods;

    // Function objects that define a typeref to this Type as its first parameter
    Array<Function*> static_methods;


    // ~~~~~~ interface ~~~~~~ 


    // handles all builtin type coersion and detecting if a type has a user defined 
    // conversion to another
    b32
    can_cast_to(Type* to);
	
	// generic function for handling the casting of whatever is at 'n'
	// to the type 'to'. Returns 0 if the cast isn't possible, otherwise
	// returns a pointer to an ASTNode that may have taken the original
	// node's place.
	// So, this should generally be used like this:
	// 		my_casted_node = thing->type->cast_to(other_thing->type, my_casted_node);
	virtual b32
	cast_to(Type* to, Expr*& n) = 0;

    u64
    hash();

    b32
    has_trait(String name);

    b32
    has_trait(Trait* trait);

    b32
    is_scalar() { return this->kind == Kind::Scalar; }

    // attempts to find the size of a given Type in bytes
    virtual u64
    size() = 0;
    
    DString
    display() = 0;

    DString
    dump() = 0;

    // given an address, return a formatted DString displaying
    // the values this Type would represent
    virtual DString
    print_from_address(u8* addr) = 0;

    Type(Kind k) : kind(k), Entity(Entity::Kind::Type) {}
};

template<> inline b32 Base::
is<Type>() { return is<Entity>() && as<Entity>()->kind == Entity::Kind::Type; }

template<> inline b32 Base::
is(Type::Kind k) { return is<Type>() && as<Type>()->kind == k; }

// type representing nothing 
struct Void : public Type { 
    Void() : Type(Type::Kind::Void) {} 

	b32      ensure_processed_to(Code::Stage stage) { return true; }
    u64      size() { return 0; }
	b32      cast_to(Type* to, Expr*& n) { return 0; } // this should never happen
    ASTNode* deep_copy() { return this; }
    DString  display() { return DString("void"); }
    DString  dump() { return DString("Void<>"); }
    DString  print_from_address(u8* addr) { return {}; } // this should never happen
	

	static const Void instance;
};

template<> inline b32 Base::
is<Void>() { return is<Type>() && as<Type>()->kind == Type::Kind::Void; }

// i dont know if this is particularly useful, I'm only using it to solve
// a specific case:
// if(...) break else ...
// 'break' is an expression, and needs a type, so I just say it is Whatever
// because it isn't going to actually return anything, it just controls flow.
// Any type may implicitly convert to this, so it's really just a wildcard
// Though it is an error if you try to use this as a value in any way
struct Whatever : public Type {
    Whatever() : Type(Type::Kind::Whatever) {}

	b32      ensure_processed_to(Code::Stage stage) { return true; }
    u64      size() { return 0; }
	b32      cast_to(Type* to, Expr*& n) { return true; }
    ASTNode* deep_copy() { return this; }
    DString  display() { return DString("whatever"); }
    DString  dump() { return DString("Whatever<>"); }
    DString  print_from_address(u8* addr) { return {}; } // this should never happen
};

template<> inline b32 Base::
is<Whatever>() { return is<Type>() && as<Type>()->kind == Type::Kind::Whatever; }

// A single number, supported by ScalarValue
struct Scalar : public Type {
	using Kind = ScalarValue::Kind;

	Kind kind;

	ScalarValue value;


    // ~~~~~~ interface ~~~~~~~

 
    DString
    display();

    DString
    dump();

	b32
	cast_to(Type* to, Expr*& n);

    u64
    size();

    DString
    print_from_address(u8* addr);

    Scalar(Kind k) : kind(k), Type(Type::Kind::Scalar) {}
};

template<> b32 inline Base::
is<Scalar>() { return is<Type>() && as<Type>()->kind == Type::Kind::Scalar; }

template<> b32 inline Base::
is(Scalar::Kind k) { return is<Scalar>() && as<Scalar>()->kind == k; };


// a Type which consists of members. Members 
// are named offsets from the base pointer of 
// a value with this Type.
// see Structure.h for further info on how Structures work
struct Structured : public Type {
	enum class Kind {
		User,
		StaticArray,
		ViewArray,
		DynamicArray,
	};

	Kind kind;

    Structure* structure;


    // ~~~~~~ interface ~~~~~~~


    static Structured*
    create(Structure* structure);

    Member*
    find_member(String id);

    DString
    display();

    DString
    dump();

	b32
	cast_to(Type* t, Expr*& e);

    u64
    size();

    DString
    print_from_address(u8* addr);

    Structured() : Type(Type::Kind::Structured) {}

    Structured(Kind k) : kind(k), Type(Type::Kind::Structured) {}
};

template<> inline b32 Base::
is<Structured>() { return is<Type>() && as<Type>()->kind == Type::Kind::Structured; }

template<> inline b32 Base::
is(Structured::Kind k) { return is<Structured>() && as<Structured>()->kind == k; }

// a Type representing an address in memory where a value of 'type' can be found 
struct Pointer : public Type {
    Type* type;

    static Array<Pointer*> set;


    // ~~~~~~ interface ~~~~~~~


    static Pointer*
    create(Type* type);

    DString
    display();

    DString
    dump();

	b32
	cast_to(Type* t, Expr*& e);

    u64
    size();

    DString
    print_from_address(u8* addr);

    Pointer() : Type(Type::Kind::Pointer) {}
};

template<> b32 inline Base::
is<Pointer>() { return is<Type>() && as<Type>()->kind == Type::Kind::Pointer; }

// NOTE(sushi) the following array types inherit from Structured, because they have
//             accessible members and thus need some Structure

// a StaticArray is an array of the form
//      T[N]
// where N is some integer. A StaticArray is allocated onto the stack
// and its data pointer and count cannot be changed, since they will
// never exist to begin with
struct StaticArray : public Structured {
    Type* type;
    u64   count;

    static Array<StaticArray*> set;


    // ~~~~~~ interface ~~~~~~~


    static StaticArray*
    create(Type* type, u64 size);

    DString
    display();

    DString
    dump();
	
	b32
	cast_to(Type* t, Expr*& e);

    u64
    size();

    DString
    print_from_address(u8* addr);

    StaticArray() : Structured(Structured::Kind::StaticArray) {}
};

template<> b32 inline Base::
is<StaticArray>() { return is<Structured>() && as<Structured>()->kind == Structured::Kind::StaticArray; }

// a DynamicArray is an array of the form
//      T[..]
// it stores the members: 
//      data: T*
//      count: u64
//      space: u64
//      allocater: $allocator // TODO(sushi)
struct DynamicArray : public Structured {
    Type* type;

    static Array<DynamicArray*> set;


    // ~~~~~~ interface ~~~~~~~


    static DynamicArray*
    create(Type* type);

    DString
    display();

    DString
    dump();

	b32
	cast_to(Type* t, Expr*& e);

    u64
    size();

    DString
    print_from_address(u8* addr);

    DynamicArray() : Structured(Structured::Kind::DynamicArray) {}
};

template<> b32 inline Base::
is<DynamicArray>() { return is<Structured>() && as<Structured>()->kind == Structured::Kind::DynamicArray;  }

// a ViewArray is the same as a StaticArray, except that it does not 
// allocate anything onto the stack and its count and data pointer 
// can be changed at runtime
// it is of the form
//      T[]
struct ViewArray : public Structured {
    Type* type;

    static Array<ViewArray*> set;


    // ~~~~~~ interface ~~~~~~~


    static ViewArray*
    create(Type* type);

    DString
    display();

    DString
    dump();

	b32
	cast_to(Type* t, Expr*& e);

    u64
    size();

    DString
    print_from_address(u8* addr);

    ViewArray() : Structured(Structured::Kind::ViewArray) {}
};

template<> b32 inline Base::
is<ViewArray>() { return is<Structured>() && as<Structured>()->kind == Structured::Kind::ViewArray;  }


struct Range : public Type {
    Type* type; // the type this Range's elements are 


    // TODO(sushi) we eventually need to make ranges unique based on their actual range 
    static Array<Range*> set;


    // ~~~~~~ interface ~~~~~~~


    static Range*
    create(Type* type);

    DString
    display();

    DString
    dump();

    u64
    size();

	b32
	cast_to(Type* t, Expr*& e);

    DString
    print_from_address(u8* addr);

    Range() : Type(Type::Kind::Range) {}
};

template<> b32 inline Base::
is<Range>() { return is<Type>() && as<Type>()->kind == Type::Kind::Range; }

// a Type which may take on the form of some collection of Types
// this is a tagged union
/* for example:
    
    IP :: variant {
        v4(u8,u8,u8,u8),
        v6(u8[]),
    }

    is equivalent to the C code:

    enum IP_Type {
        v4,
        v6
    };

    struct IP {
        IP_Type type;
        union {
            struct {
                u8 a,b,c,d;
            }v4;
            struct{
                char* str;
            }v6;
        };
    };
*/
struct Variant : public Type {
    Array<Type*> variants;
    
    static Variant*
    create();
};

namespace type::variant {
} // namespace type::variant

struct FunctionType : public Type {
    // pointers to the nodes that define these things
    ASTNode* parameters;
    ASTNode* returns;
    Type* return_type;


    // ~~~~~~ interface ~~~~~~~


    static FunctionType*
    create();

    DString
    display();

    DString
    dump();

    u64
    size();

	b32
	cast_to(Type* t, Expr*& e);

    DString
    print_from_address(u8* addr);

    FunctionType() : Type(Type::Kind::Function) {}
};

struct Element {
	Type* type;
	u64 offset; // offset in bytes from the beginning of the Tuple
};

// the underlying type of any Tuple
// TupleTypes can be seen as generalizations 
// of Structured types in that they consist
// of an ordered set of subtypes and represent 
// continguous data, but they may omit names for 
// some or all of their elements.
// TupleTypes also cannot inherit members
// from other types like Structured types can
struct TupleType : public Type {
	// a map storing the named elements of
	// this TupleType to its index
	Map<String, u64> named_elements;

	Array<Element> elements;
	
	// total size in bytes
	u64 bytes;

    static Array<TupleType*> set;


    // ~~~~~~ interface ~~~~~~~

	
	// creates a TupleType from a Tuple which has 
	// ALREADY BEEN semantically analyzed!!!
    static TupleType*
    create(Tuple* tuple);

    DString
    display();

    DString
    dump();

    u64
    size();

	b32
	cast_to(Type* t, Expr*& e);

    DString
    print_from_address(u8* addr);
	
	FORCE_INLINE u64
	n_positional() { return elements.count - named_elements.keys.count; }

    TupleType() : Type(Type::Kind::Tuple) {}
};

template<> inline b32 Base::
is<TupleType>() { return is<Type>() && as<Type>()->kind == Type::Kind::Tuple; }

namespace type::tuple {
struct ExistantTupleType {
    u64 hash;
    TupleType* ttype;
};
extern Array<ExistantTupleType> set;
} // namespace type::tuple

// an Expr which represents a Module in someway
struct ModuleType : public Type {
	Module* m;


	// ~~~~ interface ~~~~
	

	static ModuleType*
	create(Module* m = 0);

	void
	destroy();

	DString
	display();

	DString
	dump();


	// the following functions should never be invoked on this Type
	// if they are for some reason then there must be some internal bug
	u64 size() { Assert(0); return 0; }
	b32 cast_to(Type* t, Expr*& e) { Assert(0); return 0; }
	DString print_from_address(u8* addr) { Assert(0); return {}; } 

	ModuleType() : Type(Type::Kind::Module) {}
};

namespace type::meta {
enum class kind {
    place,
    structure,
    module,
    function,
    trait,
};
} // namespace type::meta

// a Type which represents information about the language
// planned to be mapped 1:1 with the types we use in the compiler
// if that's even practical!
struct MetaType : public Type {
    type::meta::kind kind;
    Structure* s;
};


} // namespace amu

#endif // AMU_TYPE_H
