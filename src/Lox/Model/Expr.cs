namespace Lox.Model;

[AstGen("Binary   : Expr left, Token op, Expr right")]
[AstGen("Grouping : Expr expression")]
[AstGen("Literal  : object? value")]
[AstGen("Unary    : Token op, Expr right")]
public abstract partial class Expr
{
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

    static Expr()
    {
        _ = Expr.SomeFunc(); // Go to definition to see output of source-generator.
    }
}

[AstGen("Expression : Expr expr")]
[AstGen("Print      : Expr expr")]
public abstract partial class Stmt
{
    static Stmt()
    {
        _ = Stmt.SomeFunc(); // Go to definition to see output of source-generator.
    }
}
