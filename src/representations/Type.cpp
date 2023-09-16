namespace amu {

b32 Type::
can_cast_to(Type* to)  { 
    if(this == to) return true;

    // the Whatever type is a wildcard that anything can cast to
    // but you cannot use something of this type, and checks for that
    // are handled elsewhere
    if(this->is<Whatever>() || to->is<Whatever>()) 
        return true;
    

    // all scalar types may coerce to each other
    // TODO(sushi) this should probably not allow float <-> int coercion in implicit cases, though
    if(to->is<Scalar>() && this->is<Scalar>())
        return true;

    // pointers may coerce freely
    // TODO(sushi) stronger rules may be safer, though
    // TODO(sushi) this is NOT safe
    if(to->is<Pointer>() && this->is<Pointer>())
        return true;

    // arrays can coerce between each other as long as a conversion exists
    // between their underlying types
    if(to->is<StaticArray>() && this->is<StaticArray>()) {
        auto ato = (StaticArray*)to;
        auto athis = (StaticArray*)this;
        if(ato->type->can_cast_to(athis->type)) return true;
    }

    // allow implicit coercion of an array to its data pointer
    // this may not be a good idea either
    if(to->is<Pointer>() && this->is<StaticArray>()) {
        auto pto = to->as<Pointer>();
        auto pthis = this->as<StaticArray>();
        return pto->type == pthis->type;
    }

    return false;
}

DString* Scalar::
display() {
    switch(this->kind) {
        case scalar::unsigned8:  return DString::create("u8");
        case scalar::unsigned16: return DString::create("u16");
        case scalar::unsigned32: return DString::create("u32");
        case scalar::unsigned64: return DString::create("u64");
        case scalar::signed8:    return DString::create("s8");
        case scalar::signed16:   return DString::create("s16");
        case scalar::signed32:   return DString::create("s32");
        case scalar::signed64:   return DString::create("s64");
        case scalar::float32:    return DString::create("f32");
        case scalar::float64:    return DString::create("f64");
    }
}

u64 Scalar::
size() {
    switch(this->kind) {
        case scalar::signed8:
        case scalar::unsigned8:  return 1;
        case scalar::signed16:
        case scalar::unsigned16: return 2;
        case scalar::float32:
        case scalar::signed32:
        case scalar::unsigned32: return 4;
        case scalar::float64:
        case scalar::signed64:
        case scalar::unsigned64: return 8;
    }
}

DString* Scalar::
dump() {
    DString* out = DString::create("ScalarType<");
    switch(this->kind) {
        case scalar::unsigned8:  out->append("u8");    break;
        case scalar::unsigned16: out->append("u16");   break;
        case scalar::unsigned32: out->append("u32");   break;
        case scalar::unsigned64: out->append("u64");   break;
        case scalar::signed8:    out->append("s8");    break;
        case scalar::signed16:   out->append("s16");   break;
        case scalar::signed32:   out->append("s32");   break;
        case scalar::signed64:   out->append("s64");   break;
        case scalar::float32:    out->append("f32");   break;
        case scalar::float64:    out->append("f64");   break;
    }
    out->append(">");
    return out;
}

DString* Scalar::
print_from_address(u8* addr) {
    DString* out = DString::create();
    switch(this->kind) {
        case scalar::unsigned8:  out->append(*(u8*)addr); break;
        case scalar::unsigned16: out->append(*(u16*)addr); break;
        case scalar::unsigned32: out->append(*(u32*)addr); break;
        case scalar::unsigned64: out->append(*(u64*)addr); break;
        case scalar::signed8:    out->append(*(s8*)addr); break;
        case scalar::signed16:   out->append(*(s16*)addr); break;
        case scalar::signed32:   out->append(*(s32*)addr); break;
        case scalar::signed64:   out->append(*(s64*)addr); break;
        case scalar::float32:    out->append(*(f32*)addr); break;
        case scalar::float64:    out->append(*(f64*)addr); break;
    }
    return out;
}

b32 Scalar::
is_signed() {
    return kind == scalar::signed8 || kind == scalar::signed16 || kind == scalar::signed32 || kind == scalar::signed64;
}

b32 Scalar::
is_float() {
    return kind == scalar::float32 || kind == scalar::float64;
}

Array<Pointer*> Pointer::set = Array<Pointer*>::create();

Pointer* Pointer::
create(Type* type) {
    auto [idx,found] = amu::array::util::
        search<Pointer*, Type*>(amu::Pointer::set, type, [](Pointer* p){ return p->type; });
    if(found) return amu::Pointer::set.read(idx);
    Pointer* nu = pool::add(compiler::instance.storage.pointer_types);
    nu->type = type;
    amu::Pointer::set.insert(idx, nu);
    return nu;
}

DString* Pointer::
display() { // !Leak
    return DString::create(ScopedDeref(type->display()).x, "*");
}

DString* Pointer::
dump() {
    return display();
}

u64 Pointer::
size() {
    return sizeof(void*);
}

DString* Pointer::
print_from_address(u8* addr) {
    return DString::create((u8*)*(u64*)addr);
}

Array<StaticArray*> StaticArray::set = Array<StaticArray*>::create();


// NOTE(sushi) I am VERY sorry to whoever reads or needs to fix the following functions 
//             I am not interested in trying to setup a concrete implementation of storing 
//             and accessing unique types yet, so the following code is stupidly scuffed
StaticArray* StaticArray::
create(Type* type, u64 count) {
    u64 hash = (u64(type) << count) * 1234;
    auto [idx, found] = amu::array::util:: // this suuuuuuuucks
        search<StaticArray*, u64>(StaticArray::set, hash, 
            [](StaticArray* a){ return (u64(a->type) << a->count) * 1234; });

    if(found) return amu::StaticArray::set.read(idx);
    StaticArray* nu = pool::add(compiler::instance.storage.static_array_types);
    nu->type = type;
    nu->count = count;
    amu::StaticArray::set.insert(idx, nu);

    auto s = Structure::create();
    
    auto data = s->add_member("data");
    data->type = Pointer::create(type);
    data->inherited = false;
    data->offset = 0;

    auto count_ = s->add_member("count");
    count_->type = &scalar::_u64;
    count_->inherited = false;
    count_->offset = data->type->size();

    nu->structure = s;

    return nu;
}

DString* StaticArray::
display() {
    return DString::create(ScopedDeref(this->type->display()).x, "[", this->count, "]"); 
}

DString* StaticArray::
dump() {
    return display();
}

u64 StaticArray::
size() {
    return type->size() * count;
}

DString* StaticArray::
print_from_address(u8* addr) {
    auto out = DString::create();
    out->append("[");
    forI(count) {
        out->append(ScopedDeref(type->print_from_address(addr + i * type->size())).x, ",");
    }
    out->append("]");
    return out;
}

Array<ViewArray*> ViewArray::set = Array<ViewArray*>::create();

ViewArray* ViewArray::
create(Type* type) {
    u64 hash = (u64)type;
    auto [idx, found] = amu::array::util::
        search<ViewArray*, u64>(ViewArray::set, hash, [](ViewArray* a){return u64(a->type);});

    if(found) return amu::ViewArray::set.read(idx);

    ViewArray* nu = pool::add(compiler::instance.storage.view_array_types);
    nu->type = type;
    amu::ViewArray::set.insert(idx, nu);

    auto s = Structure::create();

    auto data = s->add_member("data"); 
    data->type = Pointer::create(type);
    data->inherited = false;
    data->offset = 0;

    auto count = s->add_member("count");
    count->type = &scalar::_u64;
    count->inherited = false;
    count->offset = data->type->size();

    nu->structure = s;

    return nu;
}

DString* ViewArray::
display() {
    return DString::create(ScopedDeref(this->type->display()).x, "[]");
}

DString* ViewArray::
dump() {
    return display();
}

u64 ViewArray::
size() {
    return sizeof(void*) + sizeof(u64);
}

DString* ViewArray::
print_from_address(u8* addr) {
    TODO("print view array from address");
    return 0;
}

Array<DynamicArray*> DynamicArray::set = Array<DynamicArray*>::create();

DynamicArray* DynamicArray::
create(Type* type) {
    u64 hash = (u64)type;
    auto [idx, found] = amu::array::util::
        search<DynamicArray*, u64>(DynamicArray::set, hash, [](DynamicArray* a){return u64(a->type);});

    if(found) return amu::DynamicArray::set.read(idx);

    DynamicArray* nu = pool::add(compiler::instance.storage.dynamic_array_types);
    nu->type = type;
    amu::DynamicArray::set.insert(idx, nu);

    auto s = Structure::create();

    auto data = s->add_member("data");
    data->type = Pointer::create(type);
    data->inherited = false;
    data->offset = 0;

    auto count = s->add_member("count");
    count->type = &scalar::_u64;
    count->inherited = false;
    count->offset = data->type->size();
    
    auto space = s->add_member("space");
    space->type = &scalar::_u64;
    space->inherited = false;
    space->offset = data->type->size() + count->type->size();

    nu->structure = s;

    return nu;
}

DString* DynamicArray::
display() {
    return DString::create(ScopedDeref(this->type->display()).x, "[..]");
}

DString* DynamicArray::
dump() {
    return display();
}

u64 DynamicArray::
size() { // TODO(sushi) size of Allocators when they are implemented
    return sizeof(void*) + sizeof(u64) + sizeof(u64);
}

DString* DynamicArray::
print_from_address(u8* addr) {
    TODO("print dynamic array from address");
    return 0;
}

// FunctionType does not try to be unique for now
FunctionType* FunctionType::
create() {
    FunctionType* out = pool::add(compiler::instance.storage.function_types);
    out->kind = type::kind::function;
    out->ASTNode::kind = ast::entity;
    return out;
}

DString* FunctionType::
display() {
    DString* out = DString::create("("); 
    for(ASTNode* n = this->parameters->first_child(); n; n = n->next()) {
        out->append(n->resolve_type(), (n->next()? ", " : ""));
    }
    out->append(") -> ");
    for(ASTNode* n = this->returns; n; n = n->next()) {
        b32 block_next = n->next_is<Block>();
        out->append(n->resolve_type(), (block_next? "" : ", "));
        if(block_next) break;
    }
    return out;
}

DString* FunctionType::
dump() {
    return display();
}

u64 FunctionType::
size() {
    return sizeof(void*); // treated as pointers for now 
}

DString* FunctionType::
print_from_address(u8* addr) {
    return DString::create((u8*)*(u64*)addr);
}

Array<TupleType*> TupleType::set = Array<TupleType*>::create();

TupleType* TupleType::
create(Array<Type*>& types) {
    // extremely bad, awful, no good
    u64 hash = 1212515131534;
    forI(types.count) {
        hash <<= (u64)types.read(i) * 167272723;
    }
    auto [idx, found] = amu::array::util::
        search<TupleType*, u64>(TupleType::set, hash, [](TupleType* t){
            u64 hash = 1212515131534;
            forI(t->types.count) {
                hash <<= (u64)t->types.read(i) * 167272723;
            }
            return hash;
        });
    
    if(found) {
        types.destroy();
        return TupleType::set.read(idx);
    } 
    TupleType* nu = pool::add(compiler::instance.storage.tuple_types);
    nu->types = types;
    return nu;
}

DString* TupleType::
display() { // TODO(sushi) this sucks
    return DString::create(String{start->raw.str, end->raw.str - start->raw.str});
}

DString* TupleType::
dump() {
    return DString::create("TupleType<TODO>");
}

u64 TupleType::
size() {
    u64 count = 0;
    forI(types.count) {
        count += types.read(i)->size();
    }
    return count;
}

DString* TupleType::
print_from_address(u8* addr) {
    TODO("print tuples from address");
    return 0;
}

// namespace type::structure {
// Array<ExistingStructureType> set = amu::Array<ExistingStructureType>::create();
// } // namespace structure

// Structured* Structured::
// create(Structure* s) {
//     auto [idx, found] = amu::type::structure::set.util::
//         search<type::structure::ExistingStructureType, Structure*>(s, [](type::structure::ExistingStructureType& s) { return s.structure; });
//     if(found) return amu::type::structure::set.read(idx).stype;
//     type::structure::ExistingStructureType* nu = amu::structure::set.insert(idx);
//     nu->stype = pool::add(compiler::instance.storage.structured_types);
//     nu->stype->kind = type::kind::structured;
//     nu->stype->node.kind = node::type;
//     nu->stype->structure = s;
//     nu->structure = s;
//     return nu->stype;
// }

Structured* Structured::
create(Structure* s) {
    auto out = pool::add(compiler::instance.storage.structured_types);
    out->structure = s;
    return out;
}

Member* Structured::
find_member(String id) {
    return structure->find_member(id);
}

DString* Structured::
display() {
    return label->display();
}

DString* Structured::
dump() {
    return DString::create("Structured<TODO>");
}

u64 Structured::
size() {
    return structure->size;
}

DString* Structured::
print_from_address(u8* addr) {
    return structure->display_members_from_address(addr);
}

} // namespace amu

