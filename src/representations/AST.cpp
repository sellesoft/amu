namespace amu {


namespace internal {

template<void (*callback)(DString&,ASTNode*)> void
print_tree_recursive(DString& current, ASTNode* n, b32 newlines) {
    // TODO(sushi) this can't be static when things become multithreaded
    persist u32 layers = 0;
	if(newlines) forI(layers) dstring::append(current, "  ");

    if(n->child_count) dstring::append(current, "(");

	callback(current, n);
	
	layers++;
	for(ASTNode* c = n->first_child(); c; c = c->next()) {
		if(newlines) dstring::append(current, "\n");
		else dstring::append(current, " ");
		print_tree_recursive<callback>(current, c, newlines);
	}
	layers--;

	if(n->child_count) {
		dstring::append(current, ")");
	} 
} 

} // namespace internal

template<void (*callback)(DString&,ASTNode*)> DString ASTNode::
print_tree(b32 newlines) {
    DString out = dstring::init();
    internal::print_tree_recursive<callback>(out, this, newlines);
    return out;
} 

DString ASTNode::
print_tree(b32 newlines) {
    return print_tree<[](DString& s, ASTNode* n) { dstring::append(s, n->debug_str()); }>(newlines);
}

String ASTNode::
first_line(b32 line_numbers, b32 remove_leading_whitespace) {
	if(!start) return "ASTNode::first_line called, but start is null";
	
	u8* scan_left = start->raw.str;

	while(scan_left != start->code->raw.str && *scan_left != '\n') 
		scan_left--;

	// dont want to include a newline at the start
	if(scan_left != start->code->raw.str) scan_left++;

	u8* scan_right = start->raw.str;

	while(scan_right != start->code->raw.str + start->code->raw.count && *scan_right != '\n')
		scan_right++;

	String out = String{scan_left, scan_right-scan_left};

	if(remove_leading_whitespace) 
		out = string::skip_whitespace(out);

	if(line_numbers) // !Leak
		out = dstring::init(start->l0, ": ", out);

	return out;
}

String ASTNode::
lines() {
	if(!start || !end) return "ASTNode::lines called, but either start or end are null";

	u8* scan_left = start->raw.str;

	while(scan_left != start->code->raw.str && *scan_left != '\n') 
		scan_left--;

	u8* scan_right = end->raw.str;

	while(scan_right != start->code->raw.str + start->code->raw.count && *scan_right != '\n')
		scan_right++;

	return String{scan_left, scan_right-scan_left};
}

DString ASTNode::
underline() {
	if(!start || !end) return dstring::init("ASTNode::underline called, but either start or end are null");

	String line = first_line();

	// TODO(sushi) handle nodes spanning multiple lines in a better way 
	u64 start_depth = start->raw.str - line.str;
	u64 end_depth = util::Min(end->raw.str + end->raw.count - line.str, line.count);

	DString out = dstring::init(line, "\n");

	// TODO(sushi) this does not handle non u8 codepoints
	forI(start_depth) {
		dstring::append(out, " ");
	}

	forI(end_depth - start_depth) {
		dstring::append(out, "~");
	}

	return out;
}

} // namespace amu