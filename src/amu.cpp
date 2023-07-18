/* amu
	TODO(sushi) rewrite description of amu
*/


/* NOTES
	Following are some notes about designing the compiler

	No type will ever have a constructor that initializes memory nor a deconstructor that deinitializes memory. This functionality is to be 
	implemented through explicit 'init' and 'deinit' functions. Really, no dynamic memory manipulation may ever be done through either of these.
	Constructors should primarily be used for conversions or simple initialization of a struct with arguments, for example, String heavily uses
	constructors for compile time creation from string literals and implicit conversion from DString to String.
	
	The C++ std library should only be used in cases where platform specific functionality is required. Otherwise, amu prioritizes implementing 
	things on its own.

	Member functions should be almost always be avoided in favor of putting this functionality behind a namespace
	populated with equivalent functions taking a reference to the thing to be called on as the first argument.

*/

/*	TODOs

	syntax
		[priority (!), difficulty (*), date made, tags (optional)] (TODO)
		priority is relative to other todos, if its really not important you can use 0 instead
		difficulty can be seen as 
		*    : easy
		**   : medium
		***  : hard
		***+ : even harder/tedious
		its really meant to be taken as an abstract idea of how hard a task will be at first glance
		extra information about a TODO can be placed underneath the todo, tabbed

	try and keep it so that bugs come below features in each section, so that there is a clear separation
	if this doesnt look good or work well, we can just have a features and bugs section in each section 

	Lexer
	----- TODOs that should mainly involve working in the lexer
	

	Preprocessor
	------------ TODOs that should mainly involve working in the preprocessor
	

	Parser
	------ TODOs that should mainly involve working in the parser
	[!!,***, 2023/07/03, system] rewrite to be multithreaded on global label declarations 

	SemanticAnalyzer
	--------- TODOs that should mainly involve working in the semantic_analyzer
	  type checking
	  identifier validation

	Assembler
	--------- TODOs that should mainly involve working in the assembler
	[0,   **, 2022/07/25, feature] assemble amu to C code. 
	[!!, ***, 2022/07/25, feature] assemble amu to x86_64 assembly. 
	[!,  ***, 2022/07/25, feature] assemble amu to LLVM bytecode
	[!!, ***, 2022/07/25, feature] assemble amu to nasau's instruction set
	[0, ****, 2022/07/25, feature] make a general interface for all of these?


	Compiler
	-------- TODOs that deal with working on the compiler singleton struct

	Build System
	------------
	[!!!!, ***, 2022/08/06] implement build file interpretter 
	[!!!!, ***, 2022/08/06] decide on the build file's format
		we need to decide how the build file will be layed out and its syntax. ideally its syntax is the same as the language's
		and we use simple assignments for setting options, eg. not using silly complex formats like JSON or XML.
	[!!!!, **, 2022/08/06] make the build file template and implement its output when passed the init command.
		we need to make a simple build file template, showing the most common and necessary compile options as well as a full
		build file that contains every option you can set for the compiler, preset to its default value in the compiler.
		ideally these are autogenerated off of options we have in a list or lists somewhere, such as our globals struct in types.h

	Formatter/Logger
	---------------- TODOs relating to the process of formatting and outputting messages, including its interface
	
	Other
	----- other TODOs that may involve working on several parts of the project or just dont fit in any of the other categories
	
*/


#include "Common.h"
#include "util.h"

#include <iostream>
#include <filesystem>

#include "basic/Node.h"
#include "storage/Pool.h"
#include "storage/Array.h"
#include "storage/SharedArray.h"
#include "storage/String.h"
#include "storage/DString.h"
#include "storage/Map.h"
#include "Memory.h"
#include "Token.h"
#include "Source.h"
#include "Expression.h"
#include "Statement.h"
#include "Tuple.h"
#include "Label.h"
#include "Entity.h"
#include "Type.h"
#include "Diagnostics.h"
#include "Messenger.h"
#include "Compiler.h"
#include "Result.h"
#include "Lexer.h"
#include "Parser.h"

#include "Memory.cpp"
#include "basic/Node.cpp"
#include "storage/Pool.cpp"
#include "storage/Array.cpp"
#include "storage/SharedArray.cpp"
#include "storage/DString.cpp"
#include "storage/Map.cpp"
#include "Diagnostics.cpp"
#include "Messenger.cpp"
#include "Diagnostics.cpp"
#include "Compiler.cpp"
#include "Lexer.cpp"
#include "Parser.cpp"

int main(int argc, char* argv[]){
	{using namespace amu;

		compiler::init();

		auto args = array::init<String>(argc);
		forI(argc) {
			array::push(args, string::init(argv[i]));
		}

		compiler::begin(args);

	}
  
	return 0;
}
