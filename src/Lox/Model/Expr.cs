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
[AstGen("Print      : Expr expr")]
[AstGen("Var        : Token name, Expr? initializer")]
public abstract partial class Stmt
{
    static Stmt()
    {
        _ = Stmt.SomeFunc(); // Go to definition to see output of source-generator.
    }
}
