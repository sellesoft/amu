token curt;
array<token>& tokens = lexer.tokens;

b32 parse_failed = false;

inline void token_next(u32 count = 1) {
	curt = tokens.next(count);
}
#define currtokidx u64(tokens.iter - tokens.data)

#define token_peek tokens.peek()
#define token_look_back(i) tokens.lookback(i)
#define curr_atch(tok) (curt.type == tok)

#define ParseFail(error)\
{logE("parser", error, "\n caused by token '", curt.str, "' on line ", curt.line); parse_failed = true;}


#define Expect(Token_Type)\
if(curt.type == Token_Type) 

#define ExpectGroup(Token_Type)\
if(curt.group == Token_Type) 

#define ExpectOneOf(...)\
if(match_any(curt.type, __VA_ARGS__)) 

#define ElseExpect(Token_Type)\
else if (curt.type == Token_Type) 

#define ElseExpectGroup(Token_Type)\
else if(curt.group == Token_Type) 

#define ExpectSignature(...) if(check_signature(__VA_ARGS__))
#define ElseExpectSignature(...)  else if(check_signature(__VA_ARGS__))

#define ExpectFail(error)\
else { ParseFail(error); DebugBreakpoint; } //TODO make it so this breakpoint only happens in debug mode or whatever

#define ExpectFailCode(failcode, error)\
else { ParseFail(error); failcode }

local map<Token_Type, ExpressionType> tokToExp{
	{Token_Multiplication,     Expression_BinaryOpMultiply},
	{Token_Division,           Expression_BinaryOpDivision},
	{Token_Negation,           Expression_BinaryOpMinus},
	{Token_Plus,               Expression_BinaryOpPlus},
	{Token_AND,                Expression_BinaryOpAND},
	{Token_OR,                 Expression_BinaryOpOR},
	{Token_LessThan,           Expression_BinaryOpLessThan},
	{Token_GreaterThan,        Expression_BinaryOpGreaterThan},
	{Token_LessThanOrEqual,    Expression_BinaryOpLessThanOrEqual},
	{Token_GreaterThanOrEqual, Expression_BinaryOpGreaterThanOrEqual},
	{Token_Equal,              Expression_BinaryOpEqual},
	{Token_NotEqual,           Expression_BinaryOpNotEqual},
	{Token_BitAND,             Expression_BinaryOpBitAND},
	{Token_BitOR,              Expression_BinaryOpBitOR},
	{Token_BitXOR,		       Expression_BinaryOpBitXOR},
	{Token_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{Token_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{Token_Modulo,             Expression_BinaryOpModulo},
	{Token_BitNOT,             Expression_UnaryOpBitComp},
	{Token_LogicalNOT,         Expression_UnaryOpLogiNOT},
	{Token_Negation,           Expression_UnaryOpNegate},
};

map<cstring, Node*> knownFuncs; 
map<cstring, Node*> knownVars;  
map<cstring, Node*> knownStructs;

DataType dataTypeFromToken(Token_Type type) {
	switch (type) {
		case Token_Void      : {return DataType_Void;}
		case Token_Signed8   : {return DataType_Signed8;}        
		case Token_Signed32  : {return DataType_Signed32;}     
		case Token_Signed64  : {return DataType_Signed64;}    
		case Token_Unsigned8 : {return DataType_Unsigned8;}  
		case Token_Unsigned32: {return DataType_Unsigned32;}
		case Token_Unsigned64: {return DataType_Unsigned64; }
		case Token_Float32   : {return DataType_Float32;}     
		case Token_Float64   : {return DataType_Float64;}   
		case Token_String    : {return DataType_String;}    
		case Token_Any       : {return DataType_Any;}
		case Token_Struct    : {return DataType_Structure;}    
		default: {PRINTLN("given token type is not a data type"); return DataType_Void;}
	}
}

template<class... T>
inline b32 check_signature(u32 offset, T... in) {
	return ((tokens.peek(offset++).type == in) && ...);
}

template<typename... T> inline b32
next_match(T... in) {
	return ((tokens.peek(1).type == in) || ...);
}

template<typename... T> inline b32
next_match_group(T... in) {
	return ((tokens.peek(1).group == in) || ...);
}

Struct*   structure;
Function*    function;
Scope*       scope;
Declaration* declaration;
Statement*   statement;
Expression*  expression;
Variable*    variable;

Arena arena;

inline Node* new_structure(cstring& identifier, const string& node_str = "") {
	structure = (Struct*)arena.add(Struct());
	structure->identifier   = identifier;
	structure->node.type    = NodeType_Structure;
	structure->node.comment = node_str;
	return &structure->node;
}

inline Node* new_function(cstring& identifier, const string& node_str = "") {
	function = (Function*)arena.add(Function());
	function->identifier   = identifier;
	function->node.type    = NodeType_Function;
	function->node.comment = node_str;
	return &function->node;
}

inline Node* new_scope(const string& node_str = "") {
	scope = (Scope*)arena.add(Scope());
	scope->node.type    = NodeType_Scope;
	scope->node.comment = node_str;
	return &scope->node;
}

inline Node* new_declaration(const string& node_str = "") {
	declaration = (Declaration*)arena.add(Declaration());
	declaration->node.type    = NodeType_Declaration;
	declaration->node.comment = node_str;
	return &declaration->node;
}

inline Node* new_statement(StatementType type, const string& node_str = ""){
	statement = (Statement*)arena.add(Statement());
	statement->type = type;
	statement->node.type    = NodeType_Statement;
	statement->node.comment = node_str;
	return &statement->node;
}

inline Node* new_expression(cstring& str, ExpressionType type, const string& node_str = "") {
	expression = (Expression*)arena.add(Expression());
	expression->expstr = str;
	expression->type   = type;
	expression->node.type    = NodeType_Expression;
	expression->node.comment = node_str;
	return &expression->node;
}

b32 type_check(DataType type, Node* n) {
	Expression* e = ExpressionFromNode(n);
	
	switch (e->datatype) {
		case DataType_Signed32: {
			switch (type) {
				case DataType_Signed32: {
					return true;
				}break;
				case DataType_Signed64: {
					e->datatype = DataType_Signed64;
					return true;
				}break;
				case DataType_Unsigned32: {
					e->datatype = DataType_Unsigned32;
					//e->expstr = to_string(u32(stoi(e->expstr))); //TODO maybe just do a cast node, and handle it in assembly?
					return true;
				}break;
			}
		}break;
	}
	return false;
}

inline Node* new_variable(cstring& identifier, DataType type, const string& node_str = "") {
	variable = (Variable*)arena.add(Variable());
	variable->identifier = identifier;
	variable->node.type = NodeType_Variable;
	variable->type = type;
	variable->node.comment = node_str;
	return &variable->node;
}

b32 type_check(Node* n1, Node* n2) {
	Expression* e1 = ExpressionFromNode(n1);
	Expression* e2 = ExpressionFromNode(n2);
	
	switch (e1->datatype) {
		case DataType_Signed32: {
			switch (e2->datatype) {
				case DataType_Signed32: {
					return true;
				}break;
				case DataType_Signed64: {
					e1->datatype = DataType_Signed64;
					return true;
				}break;
				case DataType_Unsigned32: {
					e1->datatype = DataType_Unsigned32;
					//e1->expstr = to_string(u32(stoi(e1->expstr))); //TODO maybe just do a cast node, and handle it in assembly?
					return true;
				}break;
			}
		}break;
	}
	return false;
}

enum ParseState_ {
	stNone        = 0,
	stInFunction  = 1 << 0,
	stInForLoop   = 1 << 1,
	stInWhileLoop = 1 << 2, 
}; typedef u32 ParseState;
ParseState pState = stNone;

#define StateSet(flag)    AddFlag(pState, flag)
#define StateUnset(flag)  RemoveFlag(pState, flag)
#define StateHas(flag)    HasFlag(pState, flag)
#define StateHasAll(flag) HasAllFlags(pState, flag)

enum ParseStage {
	psGlobal,      // <program>       :: = { ( <function> | <struct> ) }
	psStruct,      // <struct>        :: = "struct" <id> "{" { ( <declaraion> | <function> ) } "}" [<id>] ";"
	psFunction,    // <function>      :: = <type> <id> "(" [ <declaration> {"," <declaration> } ] ")" <scope>
	psScope,       // <scope>         :: = "{" { (<declaration> | <statement> | <scope>) } "}"
	psDeclaration, // <declaration>   :: = <type> <id> [ = <exp> ] ";"
	psStatement,   // <statement>     :: = "return" <exp> ";" | <exp> ";" | <scope> 
	//                                   | "if" "(" <exp> ")" <statement> [ "else" <statement> ]
	//                                   | "for" "(" [<exp>] ";" [<exp>] ";" [<exp>] ")" <statement>
	//                                   | "for" "(" <declaration> [<exp>] ";" [<exp>] ")" <statement>
	//                                   | "while" "(" <exp> ")" <statement>
	//                                   | "break" [<integer>] ";" 
	//                                   | "continue" ";"
	psExpression,  // <exp>           :: = <id> "=" <exp> | <conditional>
	psConditional, // <conditional>   :: = <logical or> | "if" "(" <exp> ")" <exp> "else" <exp> 
	psLogicalOR,   // <logical or>    :: = <logical and> { "||" <logical and> } 
	psLogicalAND,  // <logical and>   :: = <bitwise or> { "&&" <bitwise or> } 
	psBitwiseOR,   // <bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
	psBitwiseXOR,  // <bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
	psBitwiseAND,  // <bitwise and>   :: = <equality> { "&" <equality> }
	psEquality,    // <equality>      :: = <relational> { ("!=" | "==") <relational> }
	psRelational,  // <relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
	psBitshift,    // <bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
	psAdditive,    // <additive>      :: = <term> { ("+" | "-") <term> }
	psTerm,        // <term>          :: = <factor> { ("*" | "/" | "%") <factor> }
	psFactor,      // <factor>        :: = "(" <exp> ")" | <unary> <factor> | <literal> | <id> | <incdec> <id> | <id> <incdec> |  <funccall> | "if"
	//                <funccall>      :: = < id> "("[<exp> {"," < exp > }] ")"
	//                <literal>       :: = <integer> | <float> | <string>
	//                <float>         :: = { <integer> } "." <integer> { <integer> }
	//                <string>        :: = """ { <char> } """
	//                <integer>       :: = (1|2|3|4|5|6|7|8|9|0) 
	//                <char>          :: = you know what chars are
	//                <type>          :: = (u8|u32|u64|s8|s32|s64|f32|f64|str|any)
	//                <incdec>        :: = "++" | "--"
	//                <unary>         :: = "!" | "~" | "-"
}; 

template<typename... T>
Node* binopParse(Node* node, Node* ret, ParseStage next_stage, T... tokcheck) {
	token_next();
	Node* me = new_expression(curt.str, *tokToExp.at(curt.type), ExTypeStrings[*tokToExp.at(curt.type)]);
	change_parent(me, ret);
	insert_last(node, me);
	token_next();
	ret = define(next_stage, me);
	
	while (next_match(tokcheck...)) {
		token_next();
		Node* me2 = new_expression(curt.str, *tokToExp.at(curt.type), ExTypeStrings[*tokToExp.at(curt.type)]);
		token_next();
		ret = define(next_stage, node);
		change_parent(me2, me);
		change_parent(me2, ret);
		insert_last(node, me2);
		me = me2;
	}
	return me;
}

//gathers all function, struct, and global var signatures in the program
//TODO make this function take in something to declare, rather than do all globals automatically
void declare(Node* program) {
	//TODO overloaded functions have different signatures
	for (u32 i : lexer.func_decl) {
		tokens.setiter(i);
		curt = tokens[i];
		ExpectGroup(Token_Typename) {
			DataType dtype = dataTypeFromToken(curt.type);
			token_next();
			Expect(Token_Identifier) {
				Node* me = new_function(curt.str, toStr("func ", dataTypeStrs[dtype], " ", curt.str));
				insert_last(program, me);
				function->token_idx = i;
				function->type = dtype;
				token_next();
				Expect(Token_OpenParen) {
					while (next_match_group(Token_Typename)) {
						token_next();
						dtype = dataTypeFromToken(curt.type);
						token_next();
						Expect(Token_Identifier) {
							Node* var = new_variable(curt.str, dtype, toStr("var ", dataTypeStrs[dtype], " ", curt.str));
							variable->token_idx = currtokidx - 1;
							change_parent(me, var);
							function->args.add(curt.str, variable);
							knownVars.add(curt.str, var);
							//eat any possible default var stuff
							while (!next_match(Token_Comma, Token_CloseParen)) token_next();
							token_next();
						}
					}
					//HACK
					if (next_match(Token_CloseParen)) token_next();
					Expect(Token_CloseParen) { knownFuncs.add(function->identifier, me); function->token_idx = currtokidx + 1; }
					ExpectFailCode(return; , "expected a ) for func decl ", function->identifier);
				}
			}
		}
	}

	for (u32 i : lexer.struct_decl) {
		tokens.setiter(i);
		curt = tokens[i];
		token_next();
		Expect(Token_Identifier) {
			Node* me = new_structure(curt.str, toStr("struct ", curt.str));
			insert_last(program, me);
			token_next();
			structure->token_idx = i;
			Expect(Token_OpenBrace) {
				while (match_any(tokens.peek().group, Token_Typename)) {
					token_next();
					DataType dtype = dataTypeFromToken(curt.type);
					token_next();
					Expect(Token_Identifier) {
						cstring id = curt.str;
						token_next();
						Expect(Token_OpenParen) {
							//function
							//the function should have already been found above, so find it in known funcs and change its parent
							Node* f = *knownFuncs.at(id); //NOTE: this should NEVER assert, not even as some kind of syntax error
							change_parent(me, f);
							knownFuncs.remove(id); //TODO find a way to avoid doing this
							while (!next_match(Token_CloseParen)) token_next();
							token_next(); token_next();
							u32 open_count = 1;
							Expect(Token_OpenBrace) {
								//TODO this can go wrong with certain kind of syntax errors 
								//TODO maybe theres a better way to do this?
								while (open_count) {
									if      (next_match(Token_OpenBrace))  open_count++;
									else if (next_match(Token_CloseBrace)) open_count--;
									else if (next_match(Token_EOF)) ParseFail("unexpected EOF while parsing function ", structure->identifier, "::", f->identifier);
									token_next();
								}
								Expect(Token_CloseBrace) { token_next(); structure->member_funcs.add(id, FunctionFromNode(f)); }

							}
							
						}
						else ExpectOneOf(Token_Semicolon, Token_Assignment) {
							Node* var = new_variable(curt.str, dtype, toStr("var ", dataTypeStrs[dtype], " ", curt.str));
							variable->token_idx = currtokidx - 1;
							change_parent(me, var);
							structure->member_vars.add(curt.str, variable);
							//eat any possible default var stuff
							while (!next_match(Token_Comma, Token_Semicolon)) token_next();
							token_next();
						}
					}
				}
				Expect(Token_CloseBrace){
					token_next();
					Expect(Token_Semicolon) { knownStructs.add(structure->identifier, me); }
					ExpectFailCode(return;, "expected ; for struct decl ", structure->identifier);
				}ExpectFailCode(return;, "expected } for struct decl ", structure->identifier);
			}ExpectFailCode(return; , "expected { after struct identifier ", structure->identifier);
		}ExpectFailCode(return;, "expected an identifier for struct decl");
			
	}

	//TODO come up with a good way to declare all global varibles
}

Node* define(ParseStage stage, Node* node) {
	if (parse_failed) return 0;
	
	switch (stage) {
		
		case psGlobal: { ////////////////////////////////////////////////////////////////////// @Global
			while (!(curt.type == Token_EOF || next_match(Token_EOF))) {
				if (parse_failed) return 0;
				
				ExpectGroup(Token_Typename) {
					ExpectSignature(1, Token_Identifier, Token_OpenParen) {
						define(psFunction, node);
					}
					Expect(Token_Identifier){
						if (next_match(Token_OpenParen)) {
							
							
						}
					}
				}ExpectFail("yeah i dont know right now");
				token_next();
			}
		}break;

		case psStruct: {
			if (node->type == NodeType_Structure) {
				Struct* s = StructFromNode(node);
				for (Variable* v : s->member_vars) {
					define(psDeclaration, &v->node);
				}
				for (Function* f : s->member_funcs) {
					define(psFunction, &f->node);
				}
			}
			else {
				token_next();
				Expect(Token_Identifier) {
					Node* me = node;
					token_next();
					Expect(Token_OpenBrace) {
						while (match_any(tokens.peek().group, Token_Typename)) {
							token_next();
							DataType dtype = dataTypeFromToken(curt.type);
							token_next();
							Expect(Token_Identifier) {
								token_next();
								Expect(Token_OpenParen) {
									//function
									curt = tokens.prev();
									curt = tokens.prev();
									Node* ret = define(psFunction, me);
								}
								ExpectOneOf(Token_Semicolon, Token_Assignment) {
									//must be variable decl
									curt = tokens.prev();
									curt = tokens.prev();
									Node* ret = define(psDeclaration, me);
									token_next();
									Expect(Token_Semicolon) {}
									ExpectFail("missing ; after variable declaration");
								}
							}

						}

					}ExpectFail("expected {");
					return me;
				}ExpectFail("expected identifier for struct declaration");
			}
			
		}break;
		
		case psFunction: { //////////////////////////////////////////////////////////////////// @Function
			StateSet(stInFunction);
			if (node->type == NodeType_Function) {
				Function* f = FunctionFromNode(node);
				tokens.setiter(f->token_idx);
				curt = *tokens.iter;
				Expect(Token_OpenBrace) {
					define(psScope, node);
				}ExpectFail("expected {");
			}
			else {
				DataType type = dataTypeFromToken(curt.type);
				token_next();
				Expect(Token_Identifier) {
					Node* me = new_function(curt.str, toStr("func ", dataTypeStrs[type], " ", curt.str));
					insert_last(node, me);
					function->type = type;
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						ExpectGroup(Token_Typename) {
							define(psDeclaration, me);
							ExpectGroup(Token_Typename) { ParseFail("no , separating function parameters"); }
							//function->args.add(cstr_lit(""), Variable { declaration->identifier, declaration->type });
							while (next_match(Token_Comma)) {
								token_next(); token_next();
								define(psDeclaration, me);
								//function->args.add(cstr_lit(""),Variable{declaration->identifier, declaration->type});
								ExpectGroup(Token_Typename) { ParseFail("no , separating function parameters"); }
							}
							token_next();
						}
						Expect(Token_Identifier) { ParseFail("untyped identifier in function declaration's arguments"); }
						Expect(Token_CloseParen) {
							//knownFuncs.add(function->identifier, function);
							token_next();
							Expect(Token_OpenBrace) {
								define(psScope, me);
							}ExpectFail("expected {");
						}ExpectFail("expected )");
					}ExpectFail("expected (");
				}
			}
			StateUnset(stInFunction);

		}break;
		
		case psScope: { /////////////////////////////////////////////////////////////////////// @Scope
			Node* me = new_scope("scope");
			insert_last(node, &scope->node);
			while (!next_match(Token_CloseBrace)) {
				token_next();
				ExpectGroup(Token_Typename) {
					define(psDeclaration, me);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("missing ; after variable assignment")
				}
				ElseExpect(Token_OpenBrace) {
					define(psScope, me);
				}
				else {
					define(psStatement, me);
				}
				if (next_match(Token_EOF)) { ParseFail("Unexpected EOF"); return 0; }
			}
			token_next();
		}break;
		
		case psDeclaration: {////////////////////////////////////////////////////////////////// @Declaration
			if (node->type == NodeType_Variable) {
				//we are dealing with a variable that has already been declared
				//and are just checking it for assignment
				Variable* v = VariableFromNode(node);
				tokens.setiter(v->token_idx);
				curt = *tokens.iter;
				if (next_match(Token_Assignment)) {
					Node* ret = define(psExpression, node);
					return ret;
				}

			}
			else {
				DataType type = dataTypeFromToken(curt.type);
				token_next();
				new_declaration(toStr("decl ", dataTypeStrs[type], " ", curt.str));
				insert_last(node, &declaration->node);
				Expect(Token_Identifier) {
					declaration->identifier = curt.str;
					declaration->type = type;
					if (next_match(Token_Assignment)) {
						Node* ret = define(psExpression, &declaration->node);
						return ret;
					}
				}
			}
			
		}break;
		
		case psStatement: {//////////////////////////////////////////////////////////////////// @Statement
			switch (curt.type) {
				case Token_If: {
					Node* ifno = new_statement(Statement_Conditional, "if statement");
					insert_last(node, ifno);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for if statement"); break; }
						define(psExpression, ifno);
						token_next();
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								define(psStatement, ifno);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a variable in an unscoped if statement"); return 0; }
								define(psStatement, ifno);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
							if (next_match(Token_Else)) {
								token_next();
								define(psStatement, ifno);
							}
						}ExpectFail("expected )");
					}ExpectFail("expected (");
				}break;
				
				case Token_Else: {
					new_statement(Statement_Conditional, "else statement");
					insert_last(node, &statement->node);
					token_next();
					define(psStatement, &statement->node);
				}break;
				
				case Token_For: {
					StateSet(stInForLoop);
					Node* me = new_statement(Statement_For, "for statement");
					insert_last(node, me);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for for statement"); return 0; }
						ExpectGroup(Token_Typename) {
							//we are declaring a var for this for loop
							new_declaration("decl " + tokens.peek().str);
							insert_last(me, &declaration->node);
							define(psDeclaration, &declaration->node);
						}
						else {
							define(psExpression, me);
							token_next();
						}
						Expect(Token_Semicolon) {
							token_next(); 
							define(psExpression, me);
							token_next();
							Expect(Token_Semicolon) {
								token_next();
								define(psExpression, me);
								token_next();
							}ExpectFail("missing second ; in for statement");
						}ExpectFail("missing first ; in for statement");
						
						
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								define(psStatement, me);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a variable in an unscoped for statement"); return 0; }
								define(psStatement, me);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
						}ExpectFail("expected ) for for loop");
					}ExpectFail("expected ( after for");
					StateUnset(stInForLoop);
				}break;
				
				case Token_While: {
					StateSet(stInWhileLoop);
					Node* me = new_statement(Statement_While, "while statement");
					insert_last(node, me);
					token_next();
					Expect(Token_OpenParen) {
						token_next();
						Expect(Token_CloseParen) { ParseFail("missing expression for while statement"); return 0; }
						ExpectGroup(Token_Typename) { ParseFail("declaration not allowed for while condition"); return 0; }
						define(psExpression, me);
						token_next();
						Expect(Token_CloseParen) {
							token_next();
							Expect(Token_OpenBrace) {
								define(psStatement, me);
							}
							else {
								ExpectGroup(Token_Typename) { ParseFail("can't declare a variable in an unscoped for statement"); return 0; }
								define(psStatement, me);
								Expect(Token_Semicolon) { }
								ExpectFail("expected a ;");
							}
						}ExpectFail("expected ) for while");
					}ExpectFail("expected ( after while");
					StateUnset(stInWhileLoop);
				}break;
				
				case Token_Break: {
					if (!StateHas(stInWhileLoop | stInForLoop)) { ParseFail("break not allowed outside of while/for loop"); return 0; }
					Node* me = new_statement(Statement_Break, "break statement");
					insert_last(node, me);
					token_next();
				}break;
				
				case Token_Continue: {
					if (!StateHas(stInWhileLoop | stInForLoop)) { ParseFail("continue not allowed outside of while/for loop"); return 0; }
					Node* me = new_statement(Statement_Continue, "continue statement");
					insert_last(node, me);
					token_next();
				}break;
				
				case Token_Return: {
					new_statement(Statement_Return, "return statement");
					insert_last(node, &statement->node);
					token_next();
					define(psExpression, &statement->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("expected a ;");
					
				}break;
				
				case Token_OpenBrace: {
					define(psScope, node);
				}break;
				
				case Token_Semicolon: {
					//eat multiple semicolons
				}break;
				
				default: {
					new_statement(Statement_Expression, "exp statement");
					insert_last(node, &statement->node);
					define(psExpression, &statement->node);
					token_next();
					Expect(Token_Semicolon) {}
					ExpectFail("Expected a ;");
				}break;
			}
		}break;
		
		case psExpression: {/////////////////////////////////////////////////////////////////// @Expression
			switch (curt.type) {
				case Token_Identifier: {
					if (next_match(Token_Assignment)) {
						Node* id = new_expression(curt.str, Expression_IdentifierLHS, toStr(ExTypeStrings[Expression_IdentifierLHS], " ", curt.str));     
						token_next();
						Node* me = new_expression(curt.str, Expression_BinaryOpAssignment, ExTypeStrings[Expression_BinaryOpAssignment]);
						token_next();
						change_parent(me, id);
						Node* ret = define(psExpression, me);
						change_parent(me, ret);
						insert_last(node, me);
						return me;
					}
					else {
						new_expression(curt.str, ExpressionGuard_HEAD);
						Node* ret = define(psConditional, &expression->node);
						if (!ret) return 0;
						insert_last(node, ret);
						return ret;
					}
				}break;
				
				default: {
					new_expression(curt.str, ExpressionGuard_HEAD);
					Node* ret = define(psConditional, node);
					return ret;
				}break;
			}
		}break;
		
		case psConditional: {////////////////////////////////////////////////////////////////// @Conditional
			Expect(Token_If) {
				Node* me = new_expression(curt.str, Expression_TernaryConditional,  "if exp");
				insert_last(node, me);
				token_next();
				Expect(Token_OpenParen) {
					token_next();
					define(psExpression, me);
					token_next();
					Expect(Token_CloseParen) {
						token_next();
						define(psExpression, me);
						token_next();
						Expect(Token_Else) {
							token_next();
							define(psExpression, me);
							return me;
						}ExpectFail("conditional if's are required to have an else");
					}ExpectFail("expected ) for if expression")
				}ExpectFail("expected ( for if expression")
			}
			else {
				return define(psLogicalOR, node);
			}
		}break;
		
		case psLogicalOR: {//////////////////////////////////////////////////////////////////// @Logical OR
			Node* ret = define(psLogicalAND, node);
			if (!next_match(Token_OR))
				return ret;
			return binopParse(node, ret, psLogicalAND, Token_OR);
		}break;
		
		case psLogicalAND: {/////////////////////////////////////////////////////////////////// @Logical AND
			Node* ret = define(psBitwiseOR, node);
			if (!next_match(Token_AND))
				return ret;
			return binopParse(node, ret, psBitwiseOR);
		}break;
		
		case psBitwiseOR: {//////////////////////////////////////////////////////////////////// @Bitwise OR
			Node* ret = define(psBitwiseXOR, node);
			if (!next_match(Token_BitOR))
				return ret;
			return binopParse(node, ret, psBitwiseXOR, Token_BitOR);
		}break;
		
		case psBitwiseXOR: {/////////////////////////////////////////////////////////////////// @Bitwise XOR
			Node* ret = define(psBitwiseAND, node);
			if (!next_match(Token_BitXOR))
				return ret;
			return binopParse(node, ret, psBitwiseAND, Token_BitXOR);
		}break;
		
		case psBitwiseAND: {/////////////////////////////////////////////////////////////////// @Bitwise AND
			Node* ret = define(psEquality, node);
			if (!next_match(Token_BitAND))
				return ret;
			return binopParse(node, ret, psEquality, Token_BitAND);
		}break;
		
		case psEquality: {///////////////////////////////////////////////////////////////////// @Equality
			Node* ret = define(psRelational, node);
			if (!next_match(Token_NotEqual, Token_Equal))
				return ret;
			return binopParse(node, ret, psRelational, Token_NotEqual, Token_Equal);
		}break;
		
		case psRelational: {/////////////////////////////////////////////////////////////////// @Relational
			Node* ret = define(psBitshift, node);
			if (!next_match(Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual))
				return ret;
			return binopParse(node, ret, psBitshift, Token_LessThan, Token_GreaterThan, Token_LessThanOrEqual, Token_GreaterThanOrEqual);
		}break;
		
		case psBitshift: {///////////////////////////////////////////////////////////////////// @Bitshift
			Node* ret = define(psAdditive, node);
			if (!next_match(Token_BitShiftLeft, Token_BitShiftRight))
				return ret;
			return binopParse(node, ret, psAdditive, Token_BitShiftLeft, Token_BitShiftRight);
		}break;
		
		case psAdditive: {///////////////////////////////////////////////////////////////////// @Additive
			Node* ret = define(psTerm, node);
			if (!next_match(Token_Plus, Token_Negation))
				return ret;
			return binopParse(node, ret, psTerm, Token_Plus, Token_Negation);
		}break;
		
		case psTerm: {///////////////////////////////////////////////////////////////////////// @Term
			Node* ret = define(psFactor, node);
			if (!next_match(Token_Multiplication, Token_Division, Token_Modulo))
				return ret;
			return binopParse(node, ret, psFactor, Token_Multiplication, Token_Division, Token_Modulo);
		}break;
		
		case psFactor: {/////////////////////////////////////////////////////////////////////// @Factor
			switch (curt.type) {
				
				//TODO implicitly change types here when applicable, or do that where they're returned
				case Token_LiteralFloat: {
					Node* var = new_expression(curt.str, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", curt.str));
					expression->datatype = DataType_Float32;
					insert_last(node, &expression->node);
					return var;
				}break;
				case Token_LiteralInteger: {
					Node* var = new_expression(curt.str, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " ", curt.str));
					expression->datatype = DataType_Signed32;
					insert_last(node, &expression->node);
					return var;
				}break;
				
				case Token_LiteralString: {
					Node* var = new_expression(curt.str, Expression_Literal, toStr(ExTypeStrings[Expression_Literal], " \"", curt.str, "\""));
					expression->datatype = DataType_String;
					insert_last(node, &expression->node);
					return var;
				}break;
				
				case Token_OpenParen: {
					token_next();
					Node* ret = define(psExpression, &expression->node);
					change_parent(node, ret);
					token_next();
					Expect(Token_CloseParen) { return ret; }
					ExpectFail("expected a )");
				}break;
				
				case Token_Identifier: {
					if (next_match(Token_OpenParen)) {
						if (!knownFuncs.has(curt.str)) { ParseFail(toStr("unknown function ", curt.str, " referenced")); return 0; }
						Node* me = new_expression(curt.str, Expression_Function_Call, toStr(ExTypeStrings[Expression_Function_Call], " ", curt.str));
						insert_last(node, me);
						Function* callee = FunctionFromNode(*knownFuncs.at(curt.str));
						expression->datatype = callee->type;
						token_next(); token_next();
						if (callee->args.count > 0) {
							//Expect(Token_Identifier) {
							// This will be for doing func(arg = blah,...)
							//}
							
							forI(callee->args.count) {
								Node* ret = define(psExpression, me);
								Expression* e = ExpressionFromNode(ret);
								//type_check(callee->args[i], ret);
								if (ExpressionFromNode(ret)->datatype != (*(callee->args.atIdx(i)))->type) {
									ParseFail("incorrect type provided for function argument"); return 0;
								}
								token_next();
								if (i != callee->args.count - 1) {
									Expect(Token_CloseParen) { ParseFail(toStr("Not enough arguments provided for func ", callee->identifier)); return 0; }
									Expect(Token_Comma) { token_next(); }
									ExpectFail("no , between function arguments");
								}
							}
							Expect(Token_CloseParen) { }
							ExpectFail(toStr("expected ) after function call to ", callee->identifier));
							
							//TODO list what required arguments are missing 
							//ExpectFail(toStr("expected an identifier or literal as function arg to ", callee->identifier));
						}
						else {
							Expect(Token_CloseParen) {}
							ExpectFail(toStr("expected ) on function call to ", callee->identifier));
						}
						return me;
					}
					else {
						Node* var = new_expression(curt.str, Expression_IdentifierRHS, toStr(ExTypeStrings[Expression_IdentifierRHS], " ", curt.str));
						insert_last(node, var);
						if (next_match(Token_Increment, Token_Decrememnt)) {
							token_next();
							new_expression(curt.str, (curt.type == Token_Increment ? Expression_IncrementPostfix : Expression_DecrementPostfix), (curt.type == Token_Increment ? "++ post" : "-- post"));
							insert_last(node, &expression->node);
							change_parent(&expression->node, var);
							var = &expression->node;
						}
						return var;
					}
					
				}break;
				
				case Token_If: {
					return define(psConditional, &expression->node);
				}break;
				
				case Token_Increment: {
					new_expression(curt.str, Expression_IncrementPrefix, "++ pre");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Expect(Token_Identifier) {
						define(psFactor, &expression->node);
					}ExpectFail("'++' needs l-value");
					return ret;
				}break;
				
				case Token_Decrememnt: {
					new_expression(curt.str, Expression_DecrementPrefix, "-- pre");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					Expect(Token_Identifier) {
						define(psFactor, &expression->node);
					}ExpectFail("'--' needs l-value");
					return ret;
				}break;
				
				case Token_Semicolon: {
					return node;
				}break;
				
				case Token_Negation: {
					new_expression(curt.str, Expression_UnaryOpNegate, "-");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					define(psFactor, &expression->node);
					return ret;
				}break;
				
				case Token_LogicalNOT: {
					new_expression(curt.str, Expression_UnaryOpLogiNOT, "!");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					define(psFactor, &expression->node);
					return ret;
				}break;
				
				case Token_BitNOT: {
					new_expression(curt.str, Expression_UnaryOpBitComp, "~");
					insert_last(node, &expression->node);
					token_next();
					Node* ret = &expression->node;
					define(psFactor, &expression->node);
					return ret;
				}break;
				
				default: {
					ParseFail("unexpected token found in factor");
				}break;
			}
		}break;
	}
	return 0;
}

b32 suParser::parse(Program& mother) {
	arena.init(Kilobytes(10));
	
	declare(&mother.node);

	if (parse_failed) return 1;
	
	for (Node* n : knownStructs) {
		define(psStruct, n);
	}

	for (Node* n : knownFuncs) {
		define(psFunction, n);
	}

	mother.node.comment = "program";
	
	tokens.setiter(0);
	curt = tokens[0];
	//parser(psGlobal, &mother.node);
	
	return 0;//parse_failed;
}