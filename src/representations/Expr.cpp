#include "Expr.h"
#include "Variable.h"
#include "basic/Allocator.h"

namespace amu {

Expr* Expr::
create(Allocator* allocator) {
	return new (allocator->allocate(sizeof(Expr))) Expr(Expr::Kind::Null);
}

Expr* Expr::
create(Allocator* allocator, Kind kind, Type* type) {
	auto e = new (allocator->allocate(sizeof(Expr))) Expr(kind);
	e->type = type;
	return e;
}

DString Expr::
display() {
	using enum Expr::Kind;

	switch(kind) {
		case Null: return DString("Expr<Null>");
		case Identifier: return DString("Expr<Identifier:", start->raw, ">");
		case LiteralString: return DString("Expr<StringLiteral:\"", start->raw, "\">");
		case LiteralArray: {
			auto x = DString("Expr<ArrayLiteral:[");
			for(auto e = first_child<Expr>();;) {
				auto next = e->next<Expr>();
				if(next) {
					x.append(e->display(), ", ");
					e = next;
				} else {
					x.append(e->display(), "]>");
					break;
				}
			}
			return x;
		} break;
		case LiteralTuple: {
			auto x = DString("Expr<TupleLiteral:(");
			for(auto e = first_child<Expr>();;) {
				auto next = e->next<Expr>();
				if(next) {
					x.append(e->display(), ", ");
					e = next;
				} else {
					x.append(e->display(), ")>");
					break;
				}
			}
			return x;
		} break;
		case LiteralStruct: Assert(0); return {};
		case Function: {
			NotImplemented;
			return {};
		} break;
		case Type: return DString("Expr<Type:", type->display(), ">");
		case VarRef: return DString("Expr<VarRef:", varref->display(), ">");
		case Module: Assert(0); return {};
		default: {
			Assert(0);
			return {};
		} break;
	}
}

DString Expr::
dump() {
	Assert(0);
	return {};
}

Type* Expr::
resolve_type() {
	return type;
}
 
//Expr* Expr::
//create(expr::kind kind, Type* type) {
//    Expr* out = compiler::instance.storage.expressions.add();
//    out->kind = kind;
//    out->type = type;
//    return out;
//}
//
//void Expr::
//destroy() {} 
//
//DString* Expr::
//display() { // TODO(sushi) switch on expr kind
//    return DString::create("Expression");
//}
//
//DString* Expr::
//dump() {
//    DString* out = DString::create("Expr<");
//
//    switch(this->kind) {
//        case expr::typeref: {
//            out->append("typeref:", this->type);
//        } break;
//		case expr::varref: {
//			out->append("varref:", ScopedDeref(this->varref->display()).x);
//		} break;
//		case expr::moduleref: {
//			out->append("moduleref:", ScopedDeref(this->moduleref->display()).x);
//		} break;
//        case expr::identifier: {
//            out->append("id:'", this->start->raw, "'");
//        } break;
//        case expr::cast: {
//            out->append("cast to ", this->type);
//        } break;
//        default: {
//            out->append(expr::kind_strings[(u32)this->kind]);
//        } break;
//		
//    }
//
//    out->append(">");
//
//    return out;
//}
//
//Type* Expr::
//resolve_type() {
//    return type;
//}
//
//CompileTime* CompileTime::
//create(Type* type) {
//    auto out = compiler::instance.storage.comp_times.add();
//    out->frame.locals = Array<Var*>::create();
//    out->type = type;
//    return out;
//}
//
//DString* CompileTime::
//display() {
//    return DString::create("CompileTime");
//}
//
//DString* CompileTime::
//dump() {
//    auto out = DString::create("CompileTime<");
//    if(first_child()) {
//        out->append(ScopedDeref(first_child()->dump()).x);
//    } else {
//        out->append("null");
//    }
//    out->append(">");
//    return out;
//}
//
//ScalarLiteral* ScalarLiteral::
//create() {
//    auto out = compiler::instance.storage.scalar_literals.add();
//    return out;
//}
//
//DString* ScalarLiteral::
//display() { 
//    return value.display();
//}
//
//DString* ScalarLiteral::
//dump() {
//    return DString::create("ScalarLiteral<", ScopedDeref(type->display()).x, " ", ScopedDeref(value.display()).x, ">");
//}
//
//void ScalarLiteral::
//cast_to(scalar::kind k) {
//    value.cast_to(k);
//    switch(k) {
//        case scalar::unsigned64: type = &scalar::_u64; break;
//        case scalar::unsigned32: type = &scalar::_u32; break;
//        case scalar::unsigned16: type = &scalar::_u16; break;
//        case scalar::unsigned8: type = &scalar::_u8; break;
//        case scalar::signed64: type = &scalar::_s64; break;
//        case scalar::signed32: type = &scalar::_s32; break;
//        case scalar::signed16: type = &scalar::_s16; break;
//        case scalar::signed8: type = &scalar::_s8; break;
//        case scalar::float64: type = &scalar::_f64; break;
//        case scalar::float32: type = &scalar::_f32; break;
//    }
//}
//
//void ScalarLiteral::
//cast_to(Type* t) {
//    Assert(t->is<Scalar>());
//    cast_to(t->as<Scalar>()->kind);
//}
//
//b32 ScalarLiteral::
//is_signed() {
//    return value.is_signed();
//}
//
//b32 ScalarLiteral::
//is_float() {
//    return value.is_float();
//}
//
//b32 ScalarLiteral::
//is_negative() {
//    return value.is_negative();
//}
//
//StringLiteral* StringLiteral::
//create() {
//    return compiler::instance.storage.string_literals.add();
//}
//
//DString* StringLiteral::
//display() {
//    return DString::create("\"", raw, "\"");
//}
//
//DString* StringLiteral::
//dump() {
//    return DString::create("StringLiteral<", ScopedDeref(display()).x, ">");
//}
//
//ArrayLiteral* ArrayLiteral::
//create() {
//    auto out = compiler::instance.storage.array_literals.add();
//    return out;
//}
//
//DString* ArrayLiteral::
//display() {
//    auto out = DString::create();
//    out->append("[");
//    for(Expr* e = first_child<Expr>(); e; e = e->next<Expr>()) {
//        out->append(ScopedDeref(e->display()).x, ",");
//    }
//    out->append("]");
//    return out;
//}
//
//DString* ArrayLiteral::
//dump() {
//    return DString::create("ArrayLiteral<", ScopedDeref(display()).x, ">");
//}
//
//void ArrayLiteral::
//cast_to(Type* t) {
//    for(Expr* e = first_child<Expr>(); e; e = e->next<Expr>()) {
//        if(type->as<StaticArray>()->type->is<Scalar>()) {
//            if(e->is<ScalarLiteral>()) {
//                e->as<ScalarLiteral>()->cast_to(t);
//            } else {
//                auto cast = Expr::create(expr::cast);
//                cast->type = t;
//                cast->start = e->start;
//                cast->end = e->end;
//                node::insert_above(e, cast);
//            }
//        }
//    }
//}
//
//TupleLiteral* TupleLiteral::
//create() {
//	auto out = compiler::instance.storage.tuple_literals.add();
//	return out;
//}
//
//DString* TupleLiteral::
//display() {
//	auto out = DString::create();
//	out->append("(");
//	auto t = first_child<Tuple>();
//	for(ASTNode* n = t->first_child(); n; n = n->next()) {
//		if(n->is<Label>()) {
//			out->append(n->display(), ": ", n->last_child()->display());
//		} else {
//			out->append(n->display());
//		}
//		if(n->next()) {
//			out->append(", ");
//		}
//	}
//	out->append(")");
//	return out;
//}
//
//DString* TupleLiteral::
//dump() {
//	return DString::create("TupleLiteral<", ScopedDeref(display()).x, ">");
//}
//
//Block* Block::
//create() {
//    Block* out = compiler::instance.storage.blocks.add();
//    out->kind = expr::block;
//    out->table = LabelTable::create();
//    return out;
//}
//
//DString* Block::
//display() { // !Leak TODO(sushi) get this to print something nicer
//    return dump();
//}
//
//DString* Block::
//dump() {
//    return DString::create("Block<", (type? ScopedDeref(type->display()).x->fin : "unknown type"), ">");
//}
//
//Call*
//Call::create() {
//    Call* out = compiler::instance.storage.calls.add();
//    // node::init(&out->node);
//    // out->node.kind = node::expression;
//    out->kind = expr::call;
//    return out;
//}
//
//DString* Call::
//display() { // !Leak TODO(sushi) get this to print something nicer
//    return dump();
//}
//
//DString* Call::
//dump() {
//    return DString::create("Call<", 
//                (callee? ScopedDeref(callee->display()).x->fin : "null callee"), ", ", 
//                (arguments? ScopedDeref(arguments->display()).x->fin : "null args"), ">");
//}
//
//For* For::
//create() {
//    auto out = compiler::instance.storage.fors.add();
//    out->table = LabelTable::create();
//    return out;
//}
//
//DString* For::
//display() {
//    return DString::create("for(", ScopedDeref(first_child()->display()).x, ")");
//}
//
//DString* For::
//dump() {
//    return DString::create("For<", ScopedDeref(first_child()->dump()).x, ">");
//}
//
//void
//to_string(DString* start, Expr* e) {
//    start->append("Expr<");
//    // switch(e->kind) {
//    //     case expr::typeref: {
//    //         start->append("typeref:", e->type);
//    //     } break;
//    //     case expr::identifier: {
//    //         start->append("id:'", e->node.start->raw, "'");
//    //     } break;
//    //     case expr::literal: {
//    //         switch(e->node.start->kind) {
//    //             case token::literal_character: start->append("chr lit:'", e->node.start->raw, "'"); break;
//    //             case token::literal_float:     start->append("flt lit:", e->node.start->f64_val); break;
//    //             case token::literal_integer:   start->append("int lit:", e->node.start->s64_val); break;
//    //             case token::literal_string:    start->append("str lit:'", e->node.start->raw, "'"); break;
//    //         }
//    //     } break;
//    //     case expr::varref: {
//    //         start->append("varref: ", ((VarRef*)e)->place);
//    //     } break;
//    //     default: {
//    //         start->append(expr::strings[(u32)e->kind]);
//    //     } break;
//    // }
//
//    auto a = &start;
//
//    start->append(">");
//}
//
} // namespace amu
