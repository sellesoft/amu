namespace amu {
namespace expression {

global Expression*
create() {
    Expression* out = pool::add(compiler::instance.storage.expressions);
    node::init(&out->node);
    out->node.kind = node::expression;
    return out;
}

} // namespace expression

void
to_string(DString& start, Expression* e) {
    dstring::append(start, "Expr<");
    switch(e->kind) {
        case expression::typeref: {
            dstring::append(start, "typeref:", e->type);
        } break;
        case expression::identifier: {
            dstring::append(start, "id:'", e->node.start->raw, "'");
        } break;
        case expression::literal: {
            switch(e->node.start->kind) {
                case token::literal_character: dstring::append(start, "chr lit:'", e->node.start->raw, "'"); break;
                case token::literal_float:     dstring::append(start, "flt lit:", e->node.start->f64_val); break;
                case token::literal_integer:   dstring::append(start, "int lit:", e->node.start->s64_val); break;
                case token::literal_string:    dstring::append(start, "str lit:'", e->node.start->raw, "'"); break;
            }
        } break;
        default: {
            dstring::append(start, expression::strings[e->kind]);
        } break;
    }

    dstring::append(start, ">");
}

} // namespace amu