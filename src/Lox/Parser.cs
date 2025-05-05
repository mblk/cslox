using Lox.Model;

namespace Lox;

public class Parser
{
    //
    // Grammar:
    //
    // program      → statement* EOF ;
    //
    // statement    → exprStmt
    //              | printStmt ;
    //
    // exprStmt     → expression ";" ;
    //
    // printStmt    → "print" expression ";" ;
    //
    // expression   → equality ;
    //
    // equality     → comparison (( "!=" | "==" ) comparison )* ;
    //
    // comparison   → term (( ">" | ">=" | "<" | "<=" ) term )* ;
    //
    // term         → factor (( "-" | "+" ) factor )* ;
    //
    // factor       → unary (( "/" | "*" ) unary )* ;
    //
    // unary        → ( "!" | "-" ) unary
    //              | primary ;
    //
    // primary      → NUMBER | STRING | "true" | "false" | "nil"
    //              | "(" expression ")" ;

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

    //public Expr? ParseExpr()
    //{
    //    try
    //    {
    //        return Expression();
    //    }
    //    catch (ParseError parseError)
    //    {
    //        _ = parseError;
    //        return null;
    //    }
    //}

    public IReadOnlyList<Stmt> Parse()
    {
        var statements = new List<Stmt>();

        while (!IsAtEnd())
        {
            statements.Add(Statement());
        }

        return statements;
    }

    //
    // statement → exprStmt
    //           | printStmt ;
    //
    private Stmt Statement()
    {
        if (Match(TokenType.PRINT))
            return PrintStatement();

        return ExpressionStatement();
    }

    //
    // printStmt → "print" expression ";" ;
    //
    private Stmt PrintStatement()
    {
        Expr expr = Expression();
        Consume(TokenType.SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Print(expr);
    }

    //
    // exprStmt → expression ";" ;
    //
    private Stmt ExpressionStatement()
    {
        Expr expr = Expression();
        Consume(TokenType.SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
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
