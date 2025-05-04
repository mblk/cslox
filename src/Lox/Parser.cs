using System.Numerics;
using System.Text;

namespace Lox;

[Expr("Binary   : Expr left, Token op, Expr right")]
[Expr("Grouping : Expr expression")]
[Expr("Literal  : object? value")]
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

    static Expr()
    {
        _ = Expr.SomeFunc(); // Go to definition to see output of source-generator.
    }
}

public class Parser
{
    // Grammar:
    // expression → equality ;
    // equality   → comparison (( "!=" | "==" ) comparison )* ;
    // comparison → term (( ">" | ">=" | "<" | "<=" ) term )* ;
    // term       → factor (( "-" | "+" ) factor )* ;
    // factor     → unary (( "/" | "*" ) unary )* ;
    // unary      → ( "!" | "-" ) unary
    //            | primary ;
    // primary    → NUMBER | STRING | "true" | "false" | "nil"
    //            | "(" expression ")" ;

    // Grammar notation Code representation:
    // Terminal     Code to match and consume a token
    // Nonterminal  Call to that rule’s function
    // |            if or switch statement
    // * or +       while or for loop
    // ?            if statement

    public class ParseError : Exception
    {
        public Token Token { get; }

        public ParseError(Token token, string message)
            : base(message)
        {
            Token = token;
        }
    }



    private readonly IReadOnlyList<Token> _tokens;

    private int _current = 0;

    public Parser(IReadOnlyList<Token> tokens)
    {
        _tokens = tokens;
    }

    public Expr? Parse()
    {
        try
        {
            return Expression();
        }
        catch (ParseError parseError)
        {
            _ = parseError;
            return null;
        }
    }

    //
    // expression → equality ;
    //
    private Expr Expression()
    {
        return Equality();
    }

    //
    // equality → comparison (( "!=" | "==" ) comparison )* ;
    //
    private Expr Equality()
    {
        Expr expr = Comparison();

        while (Match(TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL))
        {
            Token op = Previous();
            Expr right = Comparison();
            expr = new Expr.Binary(expr, op, right);
        }

        return expr;
    }

    //
    // comparison → term (( ">" | ">=" | "<" | "<=" ) term )* ;
    //
    private Expr Comparison()
    {
        Expr expr = Term();

        while (Match(TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.LESS, TokenType.LESS_EQUAL))
        {
            Token op = Previous();
            Expr right = Term();
            expr = new Expr.Binary(expr, op, right);
        }

        return expr;
    }

    //
    // term → factor (( "-" | "+" ) factor )* ;
    //
    private Expr Term()
    {
        Expr expr = Factor();

        while (Match(TokenType.MINUS, TokenType.PLUS))
        {
            Token op = Previous();
            Expr right = Factor();
            expr = new Expr.Binary(expr, op, right);
        }

        return expr;
    }

    //
    // factor → unary (( "/" | "*" ) unary )* ;
    //
    private Expr Factor()
    {
        Expr expr = Unary();

        while (Match(TokenType.SLASH, TokenType.STAR))
        {
            Token op = Previous();
            Expr right = Unary();
            expr = new Expr.Binary(expr, op, right);
        }

        return expr;
    }

    //
    // unary → ( "!" | "-" ) unary
    //       | primary ;
    //
    private Expr Unary()
    {
        if (Match(TokenType.BANG, TokenType.MINUS))
        {
            Token op = Previous();
            Expr right = Unary();
            return new Expr.Unary(op, right);
        }

        return Primary();
    }

    //
    // primary → NUMBER | STRING | "true" | "false" | "nil"
    //         | "(" expression ")" ;
    //
    private Expr Primary()
    {
        if (Match(TokenType.TRUE)) return new Expr.Literal(true);
        if (Match(TokenType.FALSE)) return new Expr.Literal(false);
        if (Match(TokenType.NIL)) return new Expr.Literal(null);

        if (Match(TokenType.NUMBER, TokenType.STRING))
        {
            Token t = Previous();
            return new Expr.Literal(t.Literal);
        }

        if (Match(TokenType.LEFT_PAREN))
        {
            Expr expr = Expression();
            Consume(TokenType.RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        throw Error("Expect expression.");
    }





    /// <summary>
    /// Consume one token if it matches the passed token-types.
    /// </summary>
    /// <param name="tokenTypes"></param>
    /// <returns></returns>
    private bool Match(params TokenType[] tokenTypes)
    {
        foreach (var tokenType in tokenTypes)
        {
            if (Check(tokenType))
            {
                Advance();
                return true;
            }
        }

        return false;
    }

    private bool Check(TokenType tokenType)
    {
        return Peek().Type == tokenType;
    }

    private Token Advance()
    {
        var token = Peek();
        _current++;
        return token;
    }

    private Token Consume(TokenType tokenType, string errorMessage)
    {
        if (Check(tokenType))
        {
            return Advance();
        }

        throw Error(errorMessage);
    }

    private bool IsAtEnd()
    {
        return Peek().Type == TokenType.EOF;
    }

    private Token Peek()
    {
        return _tokens[_current];
    }

    private Token Previous()
    {
        return _tokens[_current - 1];
    }




    private Exception Error(string message)
    {
        return Error(Peek(), message);
    }

    private static Exception Error(Token token, string message)
    {
        Console.WriteLine($"Error: {message} at ...");
        throw new ParseError(token, message);

    }



    /// <summary>
    /// Recover from panic mode after encountering a parse error.
    /// </summary>
    private void Synchronize()
    {
        // Consume current token
        Advance();

        while (!IsAtEnd())
        {
            // Just consumed a ';' ?
            if (Previous().Type == TokenType.SEMICOLON)
                return;

            // Next token any of these?
            switch (Peek().Type)
            {
                case TokenType.CLASS:
                case TokenType.FUN:
                case TokenType.VAR:
                case TokenType.FOR:
                case TokenType.IF:
                case TokenType.WHILE:
                case TokenType.PRINT:
                case TokenType.RETURN:
                    return;
            }

            Advance();
        }
    }


}

public class InterpreterVisitor : Expr.IVisitor<object?>
{
    public class RuntimeError : Exception
    {
        public Token Token { get; }

        public RuntimeError(Token token, string message)
            :base(message)
        {
            Token = token;
        }
    }

    public InterpreterVisitor()
    {
    }

    public object? Interpret(Expr expr)
    {
        try
        {
            return Evaluate(expr);
        }
        catch (RuntimeError runtimeError)
        {
            Console.WriteLine($"RuntimeError: {runtimeError.Message}");
            //_ = runtimeError;
            return null;
        }
    }

    public object? VisitLiteral(Expr.Literal literal)
    {
        return literal.Value;
    }

    public object? VisitGrouping(Expr.Grouping grouping)
    {
        return Evaluate(grouping.Expression);
    }

    public object? VisitUnary(Expr.Unary unary)
    {
        object? right = Evaluate(unary.Right);

        T processNumber<T>(Func<double, T> func)
        {
            if (right is double rightDouble)
                return func(rightDouble);

            throw new RuntimeError(unary.Op, "Operand must be a number.");
        }

        return unary.Op.Type switch
        {
            TokenType.MINUS => processNumber(r => -r),
            TokenType.BANG => !IsTruthy(right),

            _ => throw new NotImplementedException("Op not implemented in VisitUnary"),
        };
    }

    public object? VisitBinary(Expr.Binary binary)
    {
        object? left = Evaluate(binary.Left);
        object? right = Evaluate(binary.Right);

        object? processPlus()
        {
            if (left is double leftDouble && right is double rightDouble)
                return leftDouble + rightDouble;

            if (left is string leftString && right is string rightString)
                return leftString + rightString;

            throw new RuntimeError(binary.Op, "Operands must be numbers or strings.");
        }

        T processNumbers<T>(Func<double, double, T> func)
        {
            if (left is double leftDouble && right is double rightDouble)
                return func(leftDouble, rightDouble);

            throw new RuntimeError(binary.Op, "Operands must be numbers.");
        }

        return binary.Op.Type switch
        {
            TokenType.PLUS => processPlus(),
            TokenType.MINUS => processNumbers((l, r) => l - r),
            TokenType.SLASH => processNumbers((l, r) => l / r),
            TokenType.STAR => processNumbers((l, r) => l * r),

            TokenType.GREATER => processNumbers((l, r) => l > r),
            TokenType.GREATER_EQUAL => processNumbers((l, r) => l >= r),
            TokenType.LESS => processNumbers((l, r) => l < r),
            TokenType.LESS_EQUAL => processNumbers((l, r) => l <= r),

            TokenType.EQUAL_EQUAL => IsEqual(left, right),
            TokenType.BANG_EQUAL => !IsEqual(left, right),

            _ => throw new NotImplementedException("Op not implemented in VisitBinary"),
        };
    }

    private object? Evaluate(Expr expr)
    {
        return expr.Accept(this);
    }

    private static bool IsTruthy(object? value) // TODO move to extension/helper class?
    {
        // falsey: nil, false
        // truthy: everything else

        if (value is null) return false;
        if (value is bool b) return b;

        return true;
    }

    private static bool IsEqual(object? a, object? b) // TODO move to extension/helper class?
    {
        if (a is null && b is null) return true;
        if (a is null) return false;

        return a.Equals(b);
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