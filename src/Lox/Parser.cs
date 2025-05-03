using System.Text;

namespace Lox;

[Expr("Binary   : Expr left, Token op, Expr right")]
[Expr("Grouping : Expr expression")]
[Expr("Literal  : object value")]
[Expr("Unary    : Token op, Expr right")]
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
}

public class Parser
{
    public Parser()
    {
        _ = Expr.SomeFunc(); // Go to definition to see output of source-generator.

        Expr expression = new Expr.Binary(
            new Expr.Unary(
                new Token(TokenType.MINUS, "-", null),
                new Expr.Literal(123)),
            new Token(TokenType.STAR, "*", null),
            new Expr.Grouping(
                new Expr.Literal(45.67)));

        var v = new PrintVisitor();
        var s = expression.Accept(v);

        Console.WriteLine($"Print: {s}");


    }
}

public class PrintVisitor : Expr.IVisitor<string>
{
    public string VisitBinary(Expr.Binary binary)
    {
        return FormatAndAcceptChilds($"Binary {binary.Op.Lexeme}", binary.Left, binary.Right);
    }

    public string VisitGrouping(Expr.Grouping grouping)
    {
        return FormatAndAcceptChilds($"Grouping", grouping.Expression);
    }

    public string VisitLiteral(Expr.Literal literal)
    {
        return $"Literal {literal.Value ?? "nil"}";
    }

    public string VisitUnary(Expr.Unary unary)
    {
        return FormatAndAcceptChilds($"Unary {unary.Op.Lexeme}", unary.Right);
    }

    private string FormatAndAcceptChilds(string name, params Expr[] exprs)
    {
        var sb = new StringBuilder();

        sb.Append('(').Append(name);

        foreach (var expr in exprs)
        {
            sb.Append(' ');
            sb.Append(expr.Accept(this));
        }

        sb.Append(')');

        return sb.ToString();
    }
}