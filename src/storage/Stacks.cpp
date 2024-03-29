#include "Stacks.h"
#include "representations/Label.h"

namespace amu {

void NodeStack::
push(ASTNode* n) {
	stack.push(current);
	current = n;
}

ASTNode* NodeStack::
pop() {
	ASTNode* save = current;
	current = stack.pop();
	return save;
}

void TableStack::
push(LabelTable* l) {
	stack.push(current);
	current = l;
}

void TableStack::
pop() {
	current = stack.pop();
}

void TableStack::
add(String id, Label* l) {
	current->map.add(id, l);
}

Label* TableStack::
search(u64 hash) {
	LabelTable* table = current;
	while(table) {
		auto l = table->search(hash);
		if(l) return l;
		table = table->last;
	}
	return 0;
}

Label* TableStack::
search_local(u64 hash) {
	return current->search(hash);
}

} // namespace amu
