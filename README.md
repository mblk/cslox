# cslox - C# (and C) implementation of the LOX language

This repository contains my implementation of the [lox language] following the book [Crafting Interpreters] by [Bob Nystrom].

[Crafting Interpreters]: https://craftinginterpreters.com
[lox language]: https://craftinginterpreters.com/the-lox-language.html
[Bob Nystrom]: https://github.com/munificent

## Status

[![Build and test](https://github.com/mblk/cslox/actions/workflows/dotnet.yml/badge.svg)](https://github.com/mblk/cslox/actions/workflows/dotnet.yml)

- Phase 1: Tree walking interpreter
    - [x] REPL
    - [x] Lexing/Scanning
    - [x] Parsing (Recursive descent)
    - [x] Source generator for AST classes
    - [x] Visitor pattern
    - [x] Expression evaluation
    - [x] Print/expression statements
    - [x] Global variables
    - [x] Block statements + environments/variable shadowing
    - [x] Test runner
    - [x] If statements
    - [x] Logical operators
    - [x] While/for loops
    - [x] Break/continue
    - [x] Functions
    - [x] Anonymous functions
    - [x] Semantic analysis
    - [x] Classes
    - [x] Inheritance
    - [ ] ...
- Phase 2: Bytecode & VM
    - [x] Chunks of bytecode
    - [x] Basic VM
    - [x] REPL
    - [x] Scanning
    - [x] Parsing (Pratt + recursive descent)
    - [x] Compiling
    - [x] Value types (nil, bool, number)
    - [x] String values
    - [x] Hashtable with value_t as key and value
    - [x] Print/expression statements
    - [x] Global variables
    - [x] Block statements + local variables
    - [x] Const local variables
    - [x] Test runner (Idea: include in CI-run)
    - [x] If statements
    - [x] Logical operators
    - [x] While statements
    - [x] For statements
    - [x] Ternary operator
    - [x] Switch statements
    - [x] break/continue for loops
    - [x] Functions and calls
    - [ ] Anonymous functions
    - [x] Native functions
    - [x] Closures and upvalues
    - [ ] Garbage collector
    - [ ] ...

## Building / Running

cslox:
```
$ cd src/Lox/
$ dotnet run
```

clox debug mode:
```
$ cd src/clox/
$ make
$ ./clox ../scripts/test.lox
```

clox in release mode:
```
$ cd src/clox/
$ make BUILD=release
$ ./clox ../scripts/test.lox
```

## Using the REPL

cslox:
```
$ cd src/Lox/
$ dotnet run
> print 1 + 2;
3
>
(CTRL-C)
$
```

## Interpreting lox source files

cslox:
```
$ cd src/Lox/
$ dotnet run -- ..\..\scripts\test.lox
hello lox!
$
```

## Features

