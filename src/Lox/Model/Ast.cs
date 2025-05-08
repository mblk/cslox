﻿namespace Lox.Model;

// The source-generator creates code like this:
//public interface IVisitor<T>
//{
//    T VisitBinary(Binary binary);
//    ...
//}
//public class Binary : Expr
//{
//    public Expr Left { get; }
//    public Token Operator { get; }
//    public Expr Right { get; }
//    public Binary(Expr left, Token @operator, Expr right)
//    {
//        Left = left;
//        Operator = @operator;
//        Right = right;
//    }
//    public T Accept(IVisitor<T> visitor)
//    {
//        visitor.VisitBinary(this);
//    }
//}
//...

[AstGen("Assign   : Token name, Expr value")]
[AstGen("Binary   : Expr left, Token op, Expr right")]
[AstGen("Grouping : Expr expression")]
[AstGen("Literal  : object? value")]
[AstGen("Logical  : Expr left, Token op, Expr right")]
[AstGen("Unary    : Token op, Expr right")]
[AstGen("Variable : Token name")]
[AstGen("Call     : Expr callee, Token paren, IReadOnlyList<Expr> arguments")]
public abstract partial class Expr
{
    static Expr()
    {
        _ = Expr.SomeFunc(); // Go to definition to see output of source-generator.
    }
}

[AstGen("Block      : IReadOnlyList<Stmt> statements")]
[AstGen("Expression : Expr expr")]
[AstGen("If         : Expr condition, Stmt thenBranch, Stmt? elseBranch")]
[AstGen("While      : Expr condition, Stmt body")]
[AstGen("Control    : Token op")] // break/continue
[AstGen("Print      : Expr expr")]
[AstGen("Var        : Token name, Expr? initializer")]
[AstGen("Function   : Token name, IReadOnlyList<Token> parms, IReadOnlyList<Stmt> body")]
// TODO maybe store body as single statement like if/etc ?
//      -> difference might be 1 vs 2 environments (1 created by function call, 1 created by block) ?
[AstGen("Return     : Token token, Expr? expr")]

public abstract partial class Stmt
{
    public string? Tag { get; set; }

    static Stmt()
    {
        _ = Stmt.SomeFunc(); // Go to definition to see output of source-generator.
    }
}
