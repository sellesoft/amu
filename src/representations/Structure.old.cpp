namespace amu {

Member* Member::
create() {
    auto out = compiler::instance.storage.members.add();
    return out;
}

DString* Member::
display() {
    return label->display();
}

DString* Member::
dump() {
    return DString::create("Member<", ScopedDeref(label->display()).x, ">");
}

Structure*
Structure::create() {
    Structure* out = compiler::instance.storage.structures.add();
    out->members = Map<String, Member*>::create();
    return out;
}

Member* Structure::
find_member(String s) {
    auto [idx, found] = members.find(s);
    if(!found) return 0;
    return members.values.read(idx);
}

Member* Structure::
add_member(String id) {
    auto out = compiler::instance.storage.members.add();
    members.add(id, out);
    return out;
}

void Structure::
add_member(String id, Member* m) {
    members.add(id, m);
}

DString* Structure::
display_members_from_address(u8* start) {
    auto out = DString::create();
    for(Member* m = first_member; m; m = m->next<Member>()) {
        out->append(ScopedDeref(m->label->display()).x, ": ", ScopedDeref(m->type->print_from_address(start + m->offset)).x, "\n");
    }

    return out;
}

} // namespace amu
