# cslox - C# implementation of the LOX language

This repository contains my implementation of the [lox language] following the book [Crafting Interpreters] by [Bob Nystrom].

[Crafting Interpreters]: https://craftinginterpreters.com
[lox language]: https://craftinginterpreters.com/the-lox-language.html
[Bob Nystrom]: https://github.com/munificent


## Status

- Phase 1: Tree walking interpreter
    - [x] REPL
    - [x] Lexing/Scanning
    - [x] Parsing
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
    - [ ] Inheritance
    - [ ] ...
- Phase 2: Bytecode & VM
    - [ ] ...

## Building / Running

```bash
$ cd src/Lox/
$ dotnet run
```

## Using the REPL

```bash
$ cd src/Lox/
$ dotnet run
> print 1 + 2;
3
>
(CTRL-C)
$
```

## Interpreting lox source files

```bash
$ cd src/Lox/
$ dotnet run -- ..\..\scripts\test.lox
hello lox!
$
```

## Features

