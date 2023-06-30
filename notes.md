# amu notes

A collection of notes about work on the design of the language and the implementation of the compiler. Notes are formatted as: 
```
> **(date/time)**: (tags) <br>
note body
```
Sections may have some information at the very beginning that isn't organized into any note, and possibly throughout the rest of the section as well. 

> tags: <br>
? : something confusing, or something that I am unsure of <br>
! : a problem. if it has been solved, link to it.<br>
\* : the solution to a problem. if the problem was mentioned before, link to it.<br>
% : an idea, if something addresses this idea later, link to it. <br>
X : a note discussing why an idea will not be implemented. link to the discussed idea. <br>
O : a note discussing how an idea was implemented. link to the discussed idea. <br>

If a note block has no tags, it can be seen as a general note about something, or maybe something I find interesting.

This file is meant to be continuously added to, notes should not be deleted or changed later, with the exception of fixing grammar or other small mistakes. Categorization of notes may be freely changed, though. Notes in each section should be sorted in decending chronological order, i.e. newer notes appear first.

> **2023-04-03 12:40:04**:
Right now this doc is added to only by me (sushi), but if someone else wants to start adding to it as well, notes shouuld start specifying who wrote them, and this note be removed. 

## Table of contents
---

1.  [compiler notes](#compiler)
    1. [lexical analysis](#lexical-analysis)
    2. [syntactic analysis](#syntactic-analysis)
    3. [semantic analysis](#semantic-analysis)
2. [language notes](#language)

---

## compiler

The compiler is split into two broad phases, the frontend and the backend. The front end deals with turning given source code into an intermediate representation, which is then used by the backend to produce executable code. 

> **2023-04-10 12:32:04**: <br>
The way we interpret labels now somewhat complicates internal naming of things, but I don't think that it's too big a deal. Now to get the name of anything, we need to first check if it has a label assigned to it, and if it doesn't use its internal name.

> **2023-04-03 12:08:16**: <br>
Currently work is focused primarily on the frontend. The first backend that is planned to be implemented is one that takes our intermediate representation and converts it to LLVM bytecode, which we pass to clang, or something, to make into an executable.

### lexical analysis
Lexical analysis takes the source code, finds *lexemes* and tranforms them into *tokens*.

> **2023-04-03 12:47:17**: % <br>
The lexer is currently not multithreaded, but it probably can be. One case where it certainly makes sense is if the very first file given to the compiler is quite large. We can separate the file into different chunks and put a thread on each one. However, later on when threads are spread among many other tasks, such as lexing multiple files, and reaching later stages such as parsing, trying to assign several threads to tokenize a file may cause the lexer to slow down, as thread chunks will have to wait for plenty of other threads to finish when it could just reserve one thread and do it in one go. We should still experiment with it, though, just to see how it would go.

### syntactic analysis

### semantic analysis

---

## language
Notes about the syntax and features of the language itself.

### struct
> **2023-04-05 12:44:09**: ?! <br>
Singleton anonymous structs are somewhat difficult to implement in the syntax. Originally, I had wanted to use the syntax
> ```
> name :: struct {}
> ```
> the idea being that since variables are declared as `name : type`, and structs are decalred as `name : struct`, and anonymous struct is just `:struct`, and you chain the syntax to make what I originally wanted. However, delle brought up that we need a constant syntax and showed that in jai, they use `::`. I kept coming up with a constant syntax for amu in the back of my mind for awhile, but couldn't settle on one. When delle was writing the ant_sim prototype, he came up with the syntax 
>``` 
> name : using struct{}
>```
> I think this syntax is very nice, but it doesn't mesh well with how I interpret `using` (which I actually didn't realize until delle pointed it out to me). What this means with my current interpretation of `using` is that the struct on the right hand side will have its members taken out and put into `name`. For a struct what this means is that we are aliasing the name used to declare a variable of that type. 

### using 