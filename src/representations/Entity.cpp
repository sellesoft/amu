namespace amu {


Type*
base(Type* t);


// String
// debug_name(Type* type){
//     switch(type->kind) {
//         case type::kind::void_: {
//             return "void";
//         } break;
//         case type::kind::scalar: {
//             auto stype = (ScalarType*)type;
//             switch(stype->kind) {
//                 case type::scalar::kind::unsigned8:  return "u8";
//                 case type::scalar::kind::unsigned16: return "u16";
//                 case type::scalar::kind::unsigned32: return "u32";
//                 case type::scalar::kind::unsigned64: return "u64";
//                 case type::scalar::kind::signed8:    return "s8";
//                 case type::scalar::kind::signed16:   return "s16";
//                 case type::scalar::kind::signed32:   return "s32";
//                 case type::scalar::kind::signed64:   return "s64";
//                 case type::scalar::kind::float32:    return "f32";
//                 case type::scalar::kind::float64:    return "f64";
//             }
//         } break;

//         case type::kind::structured: {
//             auto stype = (Structured*)type;
//             // !Leak
//             DString out = dstring::init("Structured<");
//             if(stype->label) {
//                 dstring::append(out, "'", stype->label->node.start->raw, "' ");
//                 dstring::append(out, node::util::print_tree(&stype->structure->node));
//             } else {
//                 dstring::append(out, "unknown>");
//             }
//             return out;
//         } break;

//         case type::kind::function: return ((FunctionType*)this)->name();
//         case type::kind::array: return ((StaticArray*)this)->name();

//         case type::kind::pointer: {
//             auto ptype = (Pointer*)type;
//             // !Leak
//             return dstring::init(debug_name(ptype->type), "*");
//         } break;
//     }

//     return "<<type::debug_name couldn't find a name>>";
// }



namespace type::pointer {
Array<ExistantPointer> set = amu::array::init<ExistantPointer>();
} // namespace pointer


namespace type::array {
Array<ExistantArray> set = amu::array::init<ExistantArray>();


} // namespace array

namespace function {



} // namespace function

namespace type::tuple {
Array<ExistantTupleType> set;

} // namespace tuple

Pointer* Pointer::
create(Type* type) {
    auto [idx,found] = amu::array::util::
        search<type::pointer::ExistantPointer, Type*>(type::pointer::set, type, [](type::pointer::ExistantPointer& p){ return p.type; });
    if(found) return amu::array::read(type::pointer::set, idx).ptype;
    type::pointer::ExistantPointer* nu = amu::array::insert(type::pointer::set, idx);
    nu->ptype = pool::add(compiler::instance.storage.pointer_types);
    nu->ptype->kind = type::kind::pointer;
    //nu->ptype->node.kind = node::type;
    nu->ptype->type = type;
    nu->type = type;
    return nu->ptype;
}

void
to_string(DString& start, Type* t) {
    if(!t) return dstring::append(start, "Type<null>");
    dstring::append(start, 
        node::util::print_tree<[](DString& current, TNode* n) {
            dstring::append(current, ((Type*)n)->name());
        }>((TNode*)t, false));
}











} // namespace amu