﻿//string ASMBuff = "";
//bool allow_comments = 1;
//bool print_as_we_go = 1;
//
////this may not be necessary and could just be a state enum, there only ever seems to be one flag set for each layer
//struct Flags {
//	bool add  = 0;
//	bool sub  = 0;
//	bool mult = 0;
//	bool divi = 0;
//
//	bool AND            = 0;
//	bool bitAND         = 0;
//	bool OR             = 0;
//	bool bitOR          = 0;
//	bool less           = 0;
//	bool greater        = 0;
//	bool less_eq        = 0;
//	bool greater_eq     = 0;
//	bool equal          = 0;
//	bool not_equal      = 0;
//	bool modulo         = 0;
//	bool bitXOR         = 0;
//	bool bitshift_left  = 0;
//	bool bitshift_right = 0;
//
//	bool ternary_conditional = 0;
//
//	bool logi_not = 0;
//	bool bit_comp = 0;
//	bool negate   = 0;
//
//	bool factor_eval = 0;
//	bool term_eval   = 0;
//
//	string var_offset = "";
//	bool var_assignment = 1;
//};
//
////increment when a label is made so we can generate unique names
//struct {
//	u32 OR_labels     = 0;
//	u32 OR_end_labels = 0;
//
//	u32 AND_labels     = 0;
//	u32 AND_end_labels = 0;
//
//	u32 if_labels     = 0;
//	u32 if_end_labels = 0;
//	u32 else_labels   = 0;
//
//	u32 ternary_labels = 0;
//}labels;
//
//
////variable map for keeping track of variable names and their position on the stack
//map<string, u32> var_map;
//
//#define ExprFail(error)\
//std::cout << "\n\nError: " << error << "\n caused by expression '" << exp.expstr << "'" << std::endl;
//
//inline void addASMLine(string asmLine, string comment = "") {
//	
//
//	if (print_as_we_go) {
//		string out = "";
//
//		if (asmLine[asmLine.count - 1] != ':' && asmLine[0] != '.') {
//			out += "\n    " + asmLine;
//		}
//		else {
//			out += "\n" + asmLine;
//		}
//
//		if (allow_comments && comment.count > 0) {
//			if (asmLine.count > 25) {
//				out += " # " + comment;
//			}
//			else {
//				for (int i = 0; i < 25 - asmLine.count; i++) out += " ";
//				out += "# " + comment;
//			}
//		}
//
//		std::cout << out;
//		ASMBuff += out;
//	}
//	else {
//		//check if we're adding an instruction and tab if we are
//		if (asmLine[asmLine.count - 1] != ':' && asmLine[0] != '.') {
//			ASMBuff += "\n    " + asmLine;
//		}
//		else {
//			ASMBuff += "\n\n" + asmLine;
//		}
//
//		if (allow_comments && comment.count > 0) {
//			for (int i = 0; i < 16 - asmLine.count; i++) ASMBuff += " ";
//			ASMBuff += "# " + comment;
//		}
//	}
//	
//}
//
////current statement
//Statement* smt;
////current declaration
//Declaration* decl;
//void assemble_expressions(array<Expression>& expressions, s32 idx = -1) {
//	Assert(expressions.count != 0, "assemble_expression was passed an empty array");
//	Flags flags;
//
//	//u32 label_num_on_enter = label_count;
//
//	for (int i = 0; i < (idx == -1 ? expressions.count : 1); i++) {
//		
//		Expression exp = (idx == -1 ? expressions[i] : expressions[idx]);
//		switch (exp.type) {
//
//
//	
//			////////////////////////
//			////				////
//			////     Guards     ////
//			////				////
//			////////////////////////
//	
//	
//	
//			case ExpressionGuard_Assignment: {
//				//TODO this needs to look prettier later
//				if (decl) {
//					if (var_map.at(decl->identifier)) { ExprFail("attempt to redeclare a variable"); }
//					else {
//						if (exp.expressions.count != 0) {
//							var_map.add(decl->identifier, (var_map.count + 1) * 8);
//							assemble_expressions(exp.expressions);
//							addASMLine("push  %rax", "save value of variable '" + decl->identifier + "' on the stack");
//						}
//						else {
//							//case where we declare a variable but dont assign an expression to it
//							//default to 0
//							var_map.add(decl->identifier, (var_map.count + 1) * 8);
//							addASMLine("mov   $0,  %rax", "default var value to 0");
//							addASMLine("push  %rax",      "save value of variable '" + decl->identifier + "' on the stack");
//						}
//					}
//				}
//				else{
//					assemble_expressions(exp.expressions);
//					addASMLine("mov   %rax, -" + flags.var_offset + "(%rbp)", "store result into specified variable");
//				}
//				
//			}break;
//
//			case ExpressionGuard_HEAD: {
//				assemble_expressions(exp.expressions);
//			}break;
//
//			case ExpressionGuard_Conditional: {
//				assemble_expressions(exp.expressions);
//				//check if we're dealing with ternary conditional and manually handle the expressions if we are
//				//if (i < expressions.count - 1 && expressions[i + 1].type == Expression_TernaryConditional) {
//				//	assemble_expressions(exp.expressions, 0);
//				//	addASMLine("cmp   $0,   %rax", "check if last result was true for ternary conditional");
//				//	addASMLine("je    " + toStr(labels.ternary_labels++));
//				//	assemble_expressions(exp.expressions, 1);
//				//	//addASMLine()
//				//
//				//}
//
//			}break;
//	
//			case ExpressionGuard_LogicalOR: {
//				assemble_expressions(exp.expressions);
//				//peek to see if there's an OR ahead
//				if (i < expressions.count - 1 && expressions[i + 1].type == Expression_BinaryOpOR) {
//					//if there is we must check if the last result was true
//					string label_num = toStr(labels.OR_labels);
//					addASMLine("cmp   $0,   %rax", "check if last result was true for OR");
//					addASMLine("je    _ORLabel" + label_num);
//					addASMLine("mov   $1,   %rax", "we didn't jump so last result was true");
//					addASMLine("jmp   _ORend" + toStr(labels.OR_end_labels));
//					addASMLine("_ORLabel" + label_num + ":");
//					labels.OR_labels++;
//				}
//				else if (i > 0 && expressions[i - 1].type == Expression_BinaryOpOR) {
//					//if we didnt find one ahead but find one behind us then this must be the tail end of OR statements
//					addASMLine("cmp   $0,   %rax", "check if last result was true for OR");
//					addASMLine("mov   $0,   %rax", "zero out %rax and check if last result was true");
//					addASMLine("setne %al");
//					addASMLine("_ORend" + toStr(labels.OR_end_labels) + ":");
//					labels.OR_end_labels++;
//				}
//			}break;
//	
//			case ExpressionGuard_LogicalAND: {
//				assemble_expressions(exp.expressions);
//				//peek to see if there's an AND ahead
//				if (i < expressions.count - 1 && expressions[i + 1].type == Expression_BinaryOpAND) {
//					//if there is we must check if the last result was true
//					string label_num = toStr(labels.AND_labels);
//					addASMLine("cmp   $0,   %rax", "check if last result was true for AND");
//					addASMLine("jne   _ANDLabel" + label_num);
//					addASMLine("jmp   _ANDend" + toStr(labels.AND_end_labels));
//					addASMLine("_ANDLabel" + label_num + ":");
//					flags.AND = true;
//					labels.AND_labels++;
//				}
//				else if (flags.AND) {//(i > expressions.count + 1 && expressions[i - 1]->type == Expression_BinaryOpAND) {
//					//if we didnt find one ahead but find one behind us then this must be the tail end of OR statements
//					addASMLine("cmp   $0,   %rax", "check if last result was true for AND");
//					addASMLine("mov   $0,   %rax", "zero out %rax and check if last result was false");
//					addASMLine("setne %al");
//					addASMLine("_ANDend" + toStr(labels.AND_end_labels) + ":");
//					labels.AND_end_labels++;
//				}
//				
//			}break;
//	
//			case ExpressionGuard_BitOR: {
//				assemble_expressions(exp.expressions);
//				if (flags.bitOR) {
//					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise OR");
//					addASMLine("pop   %rax",       "retrieve stored from stack");
//					addASMLine("or    %rcx, %rax", "bitwise and %rax with %rcx");
//					flags.bitOR = 0;
//				}
//				
//			}break;
//	
//			case ExpressionGuard_BitXOR: {
//				assemble_expressions(exp.expressions);
//				if (flags.bitXOR) {
//					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise XOR");
//					addASMLine("pop   %rax",       "retrieve stored from stack");
//					addASMLine("xor   %rcx, %rax", "bitwise and %rax with %rcx");
//					flags.bitXOR = 0;
//				}
//	
//			}break;
//	
//			case ExpressionGuard_BitAND: {
//				assemble_expressions(exp.expressions);
//				if (flags.bitAND) {
//					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for bitwise and");
//					addASMLine("pop   %rax",       "retrieve stored from stack");
//					addASMLine("and   %rcx, %rax", "bitwise and %rax with %rcx");
//					flags.bitAND = 0;
//				}
//			}break;
//	
//			case ExpressionGuard_Equality: {
//				assemble_expressions(exp.expressions);
//				if (flags.equal) {
//					addASMLine("pop   %rcx",       "retrieve stored from stack");
//					addASMLine("cmp   %rax, %rcx", "perform equality check");
//					addASMLine("mov   $0,   %rax");
//					addASMLine("sete  %al");
//					flags.equal = 0;
//				}
//				else if (flags.not_equal) { 
//					addASMLine("pop   %rcx",       "retrieve stored from stack");
//					addASMLine("cmp   %rax, %rcx", "perform equality check");
//					addASMLine("mov   $0,   %rax");
//					addASMLine("setne %al");
//					flags.not_equal = 0;
//				}
//			}break;
//	
//			case ExpressionGuard_Relational: {
//				assemble_expressions(exp.expressions);
//				if (flags.less) {
//					addASMLine("pop   %rcx",       "retrieve stored from stack");
//					addASMLine("cmp   %rax, %rcx", "perform less than check");
//					addASMLine("mov   $0,   %rax");
//					addASMLine("setl  %al");
//					flags.less = 0;
//				}
//				else if (flags.less_eq) {
//					addASMLine("pop   %rcx",       "retrieve stored from stack");
//					addASMLine("cmp   %rax, %rcx", "perform less than eq check");
//					addASMLine("mov   $0,   %eax");
//					addASMLine("setle %al");
//					flags.less_eq = 0;
//				}
//				else if (flags.greater) {
//					addASMLine("pop   %rcx", "retrieve stored from stack");
//					addASMLine("cmp   %rax, %rcx", "perform greater than check");
//					addASMLine("mov   $0,   %eax");
//					addASMLine("setg  %al");
//					flags.greater = 0;
//				}
//				else if (flags.greater_eq) {
//					addASMLine("pop   %rcx",       "retrieve stored from stack");
//					addASMLine("cmp   %rax, %rcx", "perform greater than eq check");
//					addASMLine("mov   $0,   %rax");
//					addASMLine("setge %al");
//					flags.greater_eq = 0;
//				}
//			}break;
//	
//			case ExpressionGuard_BitShift: {
//				assemble_expressions(exp.expressions);
//				if (flags.bitshift_left) {
//					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for left bitshift");
//					addASMLine("pop   %rax",       "retrieve stored from stack");
//					addASMLine("shl   %cl, %rax",  "left bitshift %rax by %rcx");
//					flags.bitshift_left = 0;
//				}
//				else if (flags.bitshift_right) {
//					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for left bitshift");
//					addASMLine("pop   %rax",       "retrieve stored from stack");
//					addASMLine("shr   %cl, %rax",  "left bitshift %rax by %rcx");
//					flags.bitshift_right = 0;
//				}
//			}break;
//	
//			case ExpressionGuard_Additive: {
//				assemble_expressions(exp.expressions);
//				if (flags.add) {
//					addASMLine("pop   %rcx",       "retrieve stored from stack");
//					addASMLine("add   %rcx, %rax", "add, store result in %rax");
//					flags.add = 0;
//				}
//				else if (flags.sub) {
//					addASMLine("mov   %rax, %rcx", "mov %rax into %rcx for subtraction");
//					addASMLine("pop   %rax",       "retrieve stored from stack");
//					addASMLine("sub   %rcx, %rax", "sub, store result in %rax");
//					flags.sub = 0;
//				}
//				
//			}break;
//	
//			case ExpressionGuard_Term: {
//				assemble_expressions(exp.expressions);
//				if (flags.mult) {
//					addASMLine("pop   %rcx",       "retrieve stored from stack");
//					addASMLine("imul  %rcx, %rax", "signed multiply, store result in %rax");
//					flags.mult = 0;
//				}
//				else if (flags.divi) {
//					addASMLine("mov   %rax, %rcx", "swap %rax and %rcx for division");
//					addASMLine("pop   %rax",       "retrieve stored from stack");
//					addASMLine("cqto",             "convert quad in %rax to octo in %rdx:%rax");
//					addASMLine("idiv  %rcx",       "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
//					flags.divi = 0;
//				}
//				else if (flags.modulo) {
//					addASMLine("mov   %rax, %rcx", "swap %rax and %rcx for division");
//					addASMLine("pop   %rax",       "retrieve stored from stack");
//					addASMLine("cqto",             "convert quad in %rax to octo in %rdx:%rax");
//					addASMLine("idiv  %rcx",       "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
//					addASMLine("mov   %rdx, %rax", "move remainder into %rax");
//					flags.modulo = 0;
//				}
//			}break;
//	
//			case ExpressionGuard_Factor: {
//				assemble_expressions(exp.expressions);
//			}break;
//	
//	
//	
//			////////////////////////
//			////				////
//			////   Binary Ops   ////
//			////				////
//			////////////////////////
//	
//	
//	
//			case Expression_BinaryOpPlus: {
//				addASMLine("push  %rax", "store %rax for addition");
//				flags.add = 1;
//			}break;
//	
//			case Expression_BinaryOpMinus: {
//				addASMLine("push  %rax", "store %rax for subtraction");
//				flags.sub = 1;
//			}break;
//	
//			case Expression_BinaryOpMultiply: {
//				addASMLine("push  %rax", "store %rax for multiplication");
//				flags.mult = 1;
//			}break;
//	
//			case Expression_BinaryOpDivision: {
//				addASMLine("push  %rax", "store %rax for division");
//				flags.divi = 1;
//			}break;
//	
//			case Expression_BinaryOpAND: {
//				flags.AND = 1;
//	
//			}break;
//	
//			case Expression_BinaryOpBitAND: {
//				addASMLine("push  %rax", "store %rax for bit and");
//				flags.bitAND = 1;
//			}break;
//	
//			case Expression_BinaryOpOR: {
//				flags.OR= 1;
//	
//			}break;
//	
//			case Expression_BinaryOpBitOR: {
//				addASMLine("push  %rax", "store %rax for bit or");
//				flags.bitOR = 1;
//			}break;
//	
//			case Expression_BinaryOpLessThan: {
//				addASMLine("push  %rax", "store %rax for < check");
//				flags.less = 1;
//			}break;
//	
//			case Expression_BinaryOpGreaterThan: {
//				addASMLine("push  %rax", "store %rax for > check");
//				flags.greater = 1;
//			}break;
//	
//			case Expression_BinaryOpLessThanOrEqual: {
//				addASMLine("push  %rax", "store %rax for <= check");
//				flags.less_eq = 1;
//			}break;
//	
//			case Expression_BinaryOpGreaterThanOrEqual: {
//				addASMLine("push  %rax", "store %rax for >= check");
//				flags.greater_eq = 1;
//	
//			}break;
//	
//			case Expression_BinaryOpEqual: {
//				addASMLine("push  %rax", "store %rax for equal check");
//				flags.equal = 1;
//			}break;
//	
//			case Expression_BinaryOpNotEqual: {
//				addASMLine("push  %rax", "store %rax for not equal check");
//				flags.not_equal = 1;
//			}break;
//	
//			case Expression_BinaryOpModulo: {
//				addASMLine("push  %rax", "store %rax for modulo");
//				flags.modulo = 1;
//			}break;
//	
//			case Expression_BinaryOpXOR: {
//				addASMLine("push  %rax", "store %rax for xor");
//				flags.bitXOR = 1;
//			}break;
//	
//			case Expression_BinaryOpBitShiftLeft: {
//				addASMLine("push  %rax", "store %rax for left bitshift");
//				flags.bitshift_left = 1;
//			}break;
//	
//			case Expression_BinaryOpBitShiftRight: {
//				addASMLine("push  %rax", "store %rax for right bitshift");
//				flags.bitshift_right = 1;
//			}break;
//	
//			case Expression_BinaryOpAssignment: {
//				flags.var_assignment = 1;
//			}break;
//	
//			
//			////////////////////////
//			////				////
//			////   Unary  Ops   ////
//			////				////
//			////////////////////////
//
//
//			case Expression_TernaryConditional: {
//				assemble_expressions(exp.expressions);
//				flags.ternary_conditional = 1;
//			}break;
//
//
//
//	
//			////////////////////////
//			////				////
//			////   Unary  Ops   ////
//			////				////
//			////////////////////////
//	
//	
//	
//			case Expression_UnaryOpBitComp: {
//				assemble_expressions(exp.expressions);
//				addASMLine("not  %rax", "perform bitwise complement");
//			}break;
//	
//			case Expression_UnaryOpLogiNOT: {
//				assemble_expressions(exp.expressions);
//				addASMLine("cmp   $0,   %rax", "perform logical not");
//				addASMLine("mov   $0,   %rax");
//				addASMLine("sete  %al");
//			}break;
//	
//			case Expression_UnaryOpNegate: {
//				assemble_expressions(exp.expressions);
//				addASMLine("neg   %rax", "perform negation");
//			}break;
//	
//	
//	
//			////////////////////////
//			////				////
//			////    Literals    ////
//			////				////
//			////////////////////////
//	
//	
//	
//			case Expression_IntegerLiteral: {
//				addASMLine("mov   $" + exp.expstr + ",%rax", "move integer literal into %rax");
//			}break;
//	
//	
//	
//			////////////////////////
//			////				////
//			////   Identifier   ////
//			////				////
//			////////////////////////
//	
//	
//	
//			case Expression_IdentifierRHS: {
//				try {
//					flags.var_offset = toStr(*var_map.at(exp.expstr));
//					addASMLine("mov   -" + flags.var_offset + "(%rbp), %rax", "store variable '" + exp.expstr + "' value into %rax for use in an expression");
//				}
//				catch (...) { ExprFail("attempt to reference an undeclared variable"); }
//			}break;
//	
//			case Expression_IdentifierLHS: {
//				try {
//					flags.var_offset = toStr(*var_map.at(exp.expstr));
//					if (smt->type == Statement_Return) {
//					   // addASMLine("mov   -" + flags.var_offset + "(%rbp), %rax", "store variable's value into %rax for use in an expression");
//					}
//					
//				}
//				catch (...) { ExprFail("attempt to reference an undeclared variable"); }
//			}break;
//		}
//		//expressions.next();
//	}
//}
//
////keeps track of certain info used by certain kinds of statements
//
//struct {
//	bool if_started = 0;
//	string if_end_label_num = ""; //the label a chain of if statements jump to if one of their expressions is true
//	array<u32> if_label_stack; //the labels an if statement jumps to if its exp is false
//} statement_state;
//
////TODO clean up how if/else labels are kept track of here
//bool returned = false;
//
//void assemble_statement(Statement* statement) {
//	smt = statement;
//
//	bool master_if = false;
//
//	switch (statement->type) {
//		case Statement_If: {
//			//assemble if statement's expression
//			assemble_expressions(statement->expressions);
//			addASMLine("cmp   $0,   %rax", "check if result was false for if statement");
//			addASMLine("je    _IfEndLabel" + toStr(*statement_state.if_label_stack.last));
//
//			//an if statement should only have one statement to assemble
//			assemble_statement(&statement->statements[0]);
//			
//			
//		}break;
//
//		case Statement_Else: {
//			//an else should only have one statement
//			assemble_statement(&statement->statements[0]);
//		}break;
//
//		case Statement_Conditional: {
//			//if there are no statements then we are just dealing with expressions
//			if (statement->statements.count == 0) {
//				assemble_expressions(statement->expressions);
//			}
//			//if we have only one statement, it must be an if
//			else if(statement->statements.count == 1) {
//				//push if's end label num to the stack
//				statement_state.if_label_stack.add(labels.if_end_labels++);
//
//				//statement_state.if_end_label_num = toStr(labels.if_end_labels);
//				assemble_statement(&statement->statements[0]);
//
//				//add if end label for assembled if
//				addASMLine("_IfEndLabel" + toStr(*statement_state.if_label_stack.last) + ":");
//
//				statement_state.if_label_stack.pop();
//			}
//			//if we have 2 statements we must have an if and an else
//			else if (statement->statements.count == 2) {
//				if (!statement_state.if_started) {
//					//set state to know that we have started an if statement and have a final end label
//					statement_state.if_started = 1;
//					statement_state.if_end_label_num = toStr(labels.if_end_labels++);
//					statement_state.if_label_stack.add(labels.if_end_labels++);
//					master_if = 1;
//				}
//
//
//				statement_state.if_label_stack.add(labels.if_end_labels++);
//
//				//assmeble if statement
//				assemble_statement(&statement->statements[0]);
//
//				//jump to final if statement if expression was true and we are in a chain of if elses
//				addASMLine("jmp   _IfEndLabel" + toStr(*statement_state.if_label_stack.first));
//
//				//add if end label for assembled if
//				addASMLine("_IfEndLabel" + toStr(*statement_state.if_label_stack.last) + ":");
//
//				statement_state.if_label_stack.pop();
//
//				//assemble else statement
//				assemble_statement(&statement->statements[1]);
//
//				//add final if exit label if we started the if else chain
//				if (master_if) {
//					addASMLine("_IfEndLabel" + toStr(*statement_state.if_label_stack.first) + ":");
//					master_if = 0;
//					statement_state.if_started = 0;
//				}
//			}
//			else {
//				//ExprFail("A conditional statement found more than 2 statements attched to it!")
//			}
//			
//
//		}break;
//
//		case Statement_Expression: {
//			assemble_expressions(statement->expressions);
//		}break;
//
//		case Statement_Return: {
//			assemble_expressions(statement->expressions);
//			returned = true;
//			//I might want this to just be in assemble_function, but i saw a suggestion to do it here so we'll see
//			addASMLine("mov   %rbp, %rsp", "restore %rsp of caller");
//			addASMLine("pop   %rbp", "retore old %rbp");
//			addASMLine("ret\n");
//		}break;
//	}
//
//
//
//	smt = nullptr;
//}
//
//void assemble_declaration(Declaration* declaration) {
//	decl = declaration;
//	if (declaration->expressions.count != 0) {
//		assemble_expressions(declaration->expressions);
//	}
//	decl = nullptr;
//}
//
//void assemble_function(Function* func) {
//	//construct function label in asm
//	addASMLine(".global " + func->identifier);
//	addASMLine(func->identifier + ":");
//
//
//	//construct function body
//	addASMLine("push  %rbp",       "save old stack frame base");
//	addASMLine("mov   %rsp, %rbp", "current top of stack is now bottom of new stack frame");
//	for (BlockItem& block_item : func->blockitems) {
//		if (block_item.is_declaration) {
//			assemble_declaration(&block_item.declaration);
//		}
//		else {
//			assemble_statement(&block_item.statement);
//		}
//	}
//	if (!returned) {
//		addASMLine("mov   $0, %rax",   "no return statement was found so return 0 by default");
//		addASMLine("mov   %rbp, %rsp", "restore %rsp of caller");
//		addASMLine("pop   %rbp",       "retore old %rbp");
//		addASMLine("ret");
//	}
//}
//

struct Assembler{
	string output;
	b32 function_returned = false;
	b32 write_comments = true;
	u32 padding_width = 2;
	u32 instruction_width = 8;
	u32 args_width = 16;
	Type sub_expression = 0;
} assembler;


////////////////
//// @utils //// //NOTE assembly is in AT&T syntax: instr src,dest #comment
////////////////
local FORCE_INLINE void
asm_pure(const char* str){
    assembler.output += str;
}

local FORCE_INLINE void
asm_pure(const string& str){
	assembler.output += str;
}

local void
asm_instruction(const char* instruction, const char* comment){  //'pad instruction # comment\n'
    upt len_instruction = Max(strlen(instruction), upt(assembler.instruction_width + assembler.args_width + 1));
    char* str = 0;
	upt count = 0;
    if(comment && assembler.write_comments){
		str   = assembler.output.str + assembler.output.count;
		count = assembler.padding_width + len_instruction + 2 + strlen(comment) + 1;
		assembler.output.reserve(assembler.output.count + count);
        sprintf(str, "%-*s%-*s# %s\n", (int)assembler.padding_width, "", (int)len_instruction, instruction, comment);
    }else{
		str   = assembler.output.str + assembler.output.count;
		count = assembler.padding_width + len_instruction + 1;
		assembler.output.reserve(assembler.output.count + count);
        sprintf(str, "%-*s%-*s\n", (int)assembler.padding_width, "", (int)len_instruction, instruction);
    }
	assembler.output.count += count;
}

local void
asm_instruction(const char* instruction, const char* args, const char* comment){  //'pad instruction args # comment\n'
    upt len_instruction = Max(strlen(instruction), upt(assembler.instruction_width));
    upt len_args = Max(strlen(args), upt(assembler.args_width));
    char* str = 0;
	upt count = 0;
    if(comment && assembler.write_comments){
		str   = assembler.output.str + assembler.output.count;
		count = assembler.padding_width + len_instruction + len_args + 2 + strlen(comment) + 2;
		assembler.output.reserve(assembler.output.count + count);
        sprintf(str, "%-*s%-*s %-*s# %s\n", (int)assembler.padding_width, "", (int)len_instruction, instruction, (int)len_args, args, comment);
    }else{
		str   = assembler.output.str + assembler.output.count;
		count = assembler.padding_width + len_instruction + len_args + 2;
		assembler.output.reserve(assembler.output.count + count);
        sprintf(str, "%-*s%-*s %-*s\n", (int)assembler.padding_width, "", (int)len_instruction, instruction, (int)len_args, args);
    }
    assembler.output.count += count;
}

local FORCE_INLINE void
asm_start_scope(){
    asm_instruction("push", "%rbp",      "save base pointer to stack (start scope)");
    asm_instruction("mov",  "%rsp,%rbp", "put the previous stack pointer into the base pointer");
}

local FORCE_INLINE void
asm_end_scope(){
    asm_instruction("leave", "undo stack pointer move and push (end scope)");
}

local FORCE_INLINE void
asm_push_stack(u32 reg, const char* comment = 0){
	asm_instruction("push", registers_x64[reg], (comment) ? comment : "push register onto stack");
}

local FORCE_INLINE void
asm_pop_stack(u32 reg, const char* comment = 0){
	asm_instruction("pop",  registers_x64[reg], (comment) ? comment : "pop stack into register");
}

/////////////////////
//// @assembling ////
/////////////////////
void assemble_expression(Expression* expr);

local void assemble_expressions_from_expression(Expression* expr){
	for(Node* node = expr->node.first_child; ;node = node->next){
		assemble_expression(ExpressionFromNode(node));
		if(node->next == expr->node.first_child) break;
	}
}

local void assemble_expressions_from_expression_reverse(Expression* expr){
	for(Node* node = expr->node.last_child; ;node = node->prev){
		assemble_expression(ExpressionFromNode(node));
		if(node->prev == expr->node.last_child) break;
	}
}

local void
assemble_expression(Expression* expr){
	switch(expr->type){
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Guards
		case ExpressionGuard_Assignment:{
			
		}break;
		
        case ExpressionGuard_HEAD:{
			Assert(expr->node.child_count == 1, "ExpressionGuard_HEAD must have only one child node");
			assemble_expression(ExpressionFromNode(expr->node.first_child)); //TODO why does HEAD exist if its just a pass-thru?
		}break;
		
		case ExpressionGuard_Conditional:{
			Assert(expr->node.child_count >= 1, "ExpressionGuard_Conditional must have at least one child node");
			assemble_expressions_from_expression(expr);
			//TODO handle ternary expression
		}break;
		
		case ExpressionGuard_LogicalOR:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_LogicalOR must have three child nodes");
			assemble_expressions_from_expression(expr);
			//TODO handle logical or
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_LogicalAND:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_LogicalAND must have three child nodes");
			assemble_expressions_from_expression(expr);
			//TODO handle logical and
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_BitOR:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_BitOR must have three child nodes");
			assemble_expressions_from_expression(expr);
			if(assembler.sub_expression == Expression_BinaryOpBitOR){
				asm_instruction("mov", "%rax,%rcx", "mov %rax into %rcx for bitwise or");
				asm_pop_stack(Register_RAX, "pop stack into %rax for bitwise or");
				asm_instruction("or",  "%rcx,%rax", "bitwise or %rax with %rcx");
			}else{ NotImplemented; }
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_BitXOR:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_BitXOR must have three child nodes");
			assemble_expressions_from_expression(expr);
			if(assembler.sub_expression == Expression_BinaryOpBitXOR){
				asm_instruction("mov", "%rax,%rcx", "mov %rax into %rcx for bitwise xor");
				asm_pop_stack(Register_RAX, "pop stack into %rax for bitwise xor");
				asm_instruction("xor", "%rcx,%rax", "bitwise xor %rax with %rcx");
			}else{ NotImplemented; }
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_BitAND:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_BitAND must have three child nodes");
			assemble_expressions_from_expression(expr);
			if(assembler.sub_expression == Expression_BinaryOpBitAND){
				asm_instruction("mov", "%rax,%rcx", "mov %rax into %rcx for bitwise and");
				asm_pop_stack(Register_RAX, "pop stack into %rax for bitwise and");
				asm_instruction("and", "%rcx,%rax", "bitwise and %rax with %rcx");
			}else{ NotImplemented; }
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_Equality:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_Equality must have three child nodes");
			assemble_expressions_from_expression(expr);
			if      (assembler.sub_expression == Expression_BinaryOpEqual){
				asm_pop_stack(Register_RCX, "pop stack into %rcx for equal");
				asm_instruction("cmp",   "%rax,%rcx", "perform comparison, %rcx == %rax");
				asm_instruction("sete",  "%al",       "set %al if equal");
			}else if(assembler.sub_expression == Expression_BinaryOpNotEqual){
				asm_pop_stack(Register_RCX, "pop stack into %rcx for not equal");
				asm_instruction("cmp",   "%rax,%rcx", "perform comparison, %rcx != %rax");
				asm_instruction("setne", "%al",       "set %al if not equal");
			}else{ NotImplemented; }
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_Relational:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_Relational must have three child nodes");
			assemble_expressions_from_expression(expr);
			if      (assembler.sub_expression == Expression_BinaryOpLessThan){
				asm_pop_stack(Register_RCX, "pop stack into %rcx for less than");
				asm_instruction("cmp",   "%rax,%rcx", "perform comparison, %rcx < %rax");
				asm_instruction("setl",  "%al",       "set %al if less than");
			}else if(assembler.sub_expression == Expression_BinaryOpLessThanOrEqual){
				asm_pop_stack(Register_RCX, "pop stack into %rcx for less than/equal");
				asm_instruction("cmp",   "%rax,%rcx", "perform comparison, %rcx <= %rax");
				asm_instruction("setle", "%al",       "set %al if less than/equal");
			}else if(assembler.sub_expression == Expression_BinaryOpGreaterThan){
				asm_pop_stack(Register_RCX, "pop stack into %rcx for greater than");
				asm_instruction("cmp",   "%rax,%rcx", "perform comparison, %rcx > %rax");
				asm_instruction("setg",  "%al",       "set %al if greater than");
			}else if(assembler.sub_expression == Expression_BinaryOpGreaterThanOrEqual){
				asm_pop_stack(Register_RCX, "pop stack into %rcx for greater than/equal");
				asm_instruction("cmp",   "%rax,%rcx", "perform comparison, %rcx >= %rax");
				asm_instruction("setge", "%al",       "set %al if greater than/equal");
			}else{ NotImplemented; }
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_BitShift:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_BitShift must have three child nodes");
			assemble_expressions_from_expression(expr);
			if      (assembler.sub_expression == Expression_BinaryOpBitShiftLeft){
				asm_instruction("mov", "%rax,%rcx", "mov %rax into %rcx for bitshift left");
				asm_pop_stack(Register_RAX, "pop stack into %rax for bitshift left");
				asm_instruction("shl", "%cl,%rax",  "bitshift left %rax by %rcx");
			}else if(assembler.sub_expression == Expression_BinaryOpBitShiftRight){
				asm_instruction("mov", "%rax,%rcx", "mov %rax into %rcx for bitshift right");
				asm_pop_stack(Register_RAX, "pop stack into %rax for bitshift right");
				asm_instruction("shr", "%cl,%rax",  "bitshift right %rax by %rcx");
			}else{ NotImplemented; }
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_Additive:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_Additive must have three child nodes");
			assemble_expressions_from_expression(expr);
			if      (assembler.sub_expression == Expression_BinaryOpPlus){
				asm_pop_stack(Register_RCX, "pop stack into %rcx for addition");
				asm_instruction("add", "%rcx,%rax", "add, store result in %rax");
			}else if(assembler.sub_expression == Expression_BinaryOpMinus){
				asm_instruction("mov", "%rax,%rcx", "mov %rax into %rcx for subtraction");
				asm_pop_stack(Register_RAX, "pop stack into %rax for subtraction");
				asm_instruction("sub", "%rcx,%rax", "subtract, store result in %rax");
			}else{ NotImplemented; }
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_Term:{
			Assert(expr->node.child_count == 3, "ExpressionGuard_Term must have three child nodes");
			assemble_expressions_from_expression(expr);
			if      (assembler.sub_expression == Expression_BinaryOpMultiply){
				asm_pop_stack(Register_RCX, "pop stack into %rcx for multiplication");
				asm_instruction("imul", "%rcx,%rax", "signed multiply, store result in %rax");
			}else if(assembler.sub_expression == Expression_BinaryOpDivision){
				asm_instruction("mov",  "%rax,%rcx", "mov %rax into %rcx for division");
				asm_pop_stack(Register_RAX, "pop stack into %rax for division");
				asm_instruction("cqto",              "convert quad in %rax to octo in %rdx:%rax (sign extend)");
				asm_instruction("idiv", "%rcx",      "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
			}else if(assembler.sub_expression == Expression_BinaryOpModulo){
				asm_instruction("mov",  "%rax,%rcx", "mov %rax into %rcx for modulo");
				asm_pop_stack(Register_RAX, "pop stack into %rax for modulo");
				asm_instruction("cqto",              "convert quad in %rax to octo in %rdx:%rax (sign extend)");
				asm_instruction("idiv", "%rcx",      "signed divide %rdx:%rax by %rcx, quotient in %rax, remainder in %rdx");
				asm_instruction("mov",  "%rdx,%rax", "move remainder from %rdx into %rax");
			}else{ NotImplemented; }
			assembler.sub_expression = 0;
		}break;
		
		case ExpressionGuard_Factor:{
			Assert(expr->node.child_count >= 1, "ExpressionGuard_Factor must have at least one child node");
			assemble_expressions_from_expression(expr);
		}break;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Binary Operators
		case Expression_BinaryOpBitOR:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpBitOR must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for bitwise or");
			assembler.sub_expression = Expression_BinaryOpBitOR;
		}break;
		
		case Expression_BinaryOpBitXOR:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpBitXOR must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for bitwise xor");
			assembler.sub_expression = Expression_BinaryOpBitXOR;
		}break;
		
		case Expression_BinaryOpBitAND:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpBitAND must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for bitwise and");
			assembler.sub_expression = Expression_BinaryOpBitAND;
		}break;
		
		case Expression_BinaryOpEqual:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpEqual must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for equal");
			assembler.sub_expression = Expression_BinaryOpEqual;
		}break;
		
		case Expression_BinaryOpNotEqual:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpNotEqual must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for not equal");
			assembler.sub_expression = Expression_BinaryOpNotEqual;
		}break;
		
		case Expression_BinaryOpLessThan:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpLessThan must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for less than");
			assembler.sub_expression = Expression_BinaryOpLessThan;
		}break;
		
		case Expression_BinaryOpLessThanOrEqual:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpLessThanOrEqual must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for less than/equal");
			assembler.sub_expression = Expression_BinaryOpLessThanOrEqual;
		}break;
		
		case Expression_BinaryOpGreaterThan:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpGreaterThan must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for greater than");
			assembler.sub_expression = Expression_BinaryOpGreaterThan;
		}break;
		
		case Expression_BinaryOpGreaterThanOrEqual:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpGreaterThanOrEqual must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for greater than/equal");
			assembler.sub_expression = Expression_BinaryOpGreaterThanOrEqual;
		}break;
		
		case Expression_BinaryOpBitShiftLeft:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpBitShiftLeft must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for bitshift left");
			assembler.sub_expression = Expression_BinaryOpBitShiftLeft;
		}break;
		
		case Expression_BinaryOpBitShiftRight:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpBitShiftRight must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for bitshift right");
			assembler.sub_expression = Expression_BinaryOpBitShiftRight;
		}break;
		
		case Expression_BinaryOpPlus:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpPlus must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for addition");
			assembler.sub_expression = Expression_BinaryOpPlus;
		}break;
		
		case Expression_BinaryOpMinus:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpMinus must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for subtraction");
			assembler.sub_expression = Expression_BinaryOpMinus;
		}break;
		
		//NOTE direct version without guards, last child first so the stack order is correct
		/*
		case Expression_BinaryOpMinus:{
			Assert(expr->node.child_count == 2, "Expression_BinaryOpMinus must have two child nodes");
			assemble_expression(expr->node.last_child);
			asm_push_stack(Register_RAX);
			assemble_expression(expr->node.first_child);
			asm_instruction("mov", "%rax,%rcx", "mov %rax into %rcx for subtraction");
			asm_pop_stack(Register_RAX);
			asm_instruction("sub", "%rcx,%rax", "subtract, store result in %rax");
		}break;
		*/
		
		case Expression_BinaryOpMultiply:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpMultiply must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for multiplication");
			assembler.sub_expression = Expression_BinaryOpMultiply;
		}break;
		
		case Expression_BinaryOpDivision:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpDivision must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for division");
			assembler.sub_expression = Expression_BinaryOpDivision;
		}break;
		
		case Expression_BinaryOpModulo:{
			Assert(expr->node.child_count == 0, "Expression_BinaryOpModulo must have no children nodes");
			asm_push_stack(Register_RAX, "push register onto stack for modulo");
			assembler.sub_expression = Expression_BinaryOpModulo;
		}break;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Unary Operators
		case Expression_UnaryOpBitComp:{
			Assert(expr->node.child_count == 1, "Expression_UnaryOpBitComp must have only one child node");
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("not", "%rax", "perform bitwise complement");
		}break;
		
		case Expression_UnaryOpLogiNOT:{
			Assert(expr->node.child_count == 1, "Expression_UnaryOpLogiNOT must have only one child node");
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("cmp",  "$0,%rax", "perform logical not");
			asm_instruction("mov",  "$0,%rax", "");
			asm_instruction("sete", "%al",     "");
		}break;
		
		case Expression_UnaryOpNegate:{
			Assert(expr->node.child_count == 1, "Expression_UnaryOpNegate must have only one child node");
			assemble_expression(ExpressionFromNode(expr->node.first_child));
			asm_instruction("neg", "%rax", "perform artihmetic negation");
		}break;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		//// Literals
		case Expression_IntegerLiteral:{
			//string args = toStr("$",expr->integer_literal.value,",%rax");
			string args = "$" + expr->expstr + ",%rax";
			asm_instruction("mov", args.str, "move integer literal into %rax");
		}break;
		
		default:{
			NotImplemented;
		}break;
	}
}

local void
assemble_statement(Statement* stmt){
    switch(stmt->type){
        case Statement_Return:{
			for(Node* node = stmt->node.first_child; ;node = node->next){
				assemble_expression(ExpressionFromNode(node));
				if(node->next == stmt->node.first_child) break;
			}
			
            asm_end_scope();
            asm_instruction("ret", "return code pointer back to func call site");
            assembler.function_returned = true;
        }break;
    }
}

local void
assemble_declaration(Declaration* decl){
	if(decl->node.child_count == 0) return;
	
    for(Node* node = decl->node.first_child; ;node = node->next){
		Expression* expr = ExpressionFromNode(node);
		
		assemble_expression(expr);
		
		if(node->next == decl->node.first_child) break;
	}
}

local void
assemble_function(Function* func){
	if(func->node.child_count == 0) return;
    assembler.function_returned = false;
    
    asm_pure(func->identifier.str); asm_pure(":\n");
    asm_start_scope();
	for(Node* node = func->node.first_child; ;node = node->next){
		if      (node->type == NodeType_Declaration){
			assemble_declaration(DeclarationFromNode(node));
		}else if(node->type == NodeType_Statement){
			assemble_statement(StatementFromNode(node));
		}else{
			NotImplemented;
		}
		
		if(node->next == func->node.first_child) break;
	}
    if(!assembler.function_returned){
		asm_instruction("mov", "$0,%rax", "no return statement was found so return 0 by default");
        asm_end_scope();
        asm_instruction("ret",            "return code pointer back to func call site");
    }
}

////////////////////
//// @interface ////
////////////////////
b32 suAssembler::assemble(Program& program, string& assembly) {
    assembler.output.reserve(1024);
    //string filename(program->name); filename = "\"" + filename + "\""; //TODO add filename to Program
    //asm_instruction(".file", filename.str, "start of this file");
    asm_instruction(".text",               "start of code section");
    asm_instruction(".globl", "main", "marks the function 'main' as being global"); //TODO add entry point to Program
    
	for(Node* node = program.node.first_child; ;node = node->next){
		assemble_function(FunctionFromNode(node));
		if(node->next == program.node.first_child) break;
	}
	
	assembly = assembler.output;
	return true;
}