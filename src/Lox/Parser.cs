using Lox.Model;

namespace Lox;

public class Parser
{
    // =============
    // == Grammar ==
    // =============
    //
    // program      → declaration* EOF ;
    //
    // declaration  → varDecl
    //              | statement
    //              ;
    //
    // varDecl      → "var" IDENTIFIER ( "=" expression )? ";" ;
    //
    // statement    → exprStmt
    //              | ifStmt
    //              | printStmt
    //              | block
    //              ;
    //
    // exprStmt     → expression ";" ;
    //
    // ifStmt       → "if" "(" expression ")" statement ( "else" statement )? ;
    //
    // printStmt    → "print" expression ";" ;
    //
    // block        → "{" declaration* "}" ;
    //
    // expression   → assignment ;
    //
    // assignment   → IDENTIFIER "=" assignment
    //              | logic_or
    //              ;
    //
    // logic_or     → logic_and ( "or" logic_and )* ;
    //
    // logic_and    → equality ( "and" equality )* ;
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
    //              | primary
    //              ;
    //
    // primary      → "true" | "false" | "nil"
    //              | NUMBER | STRING
    //              | "(" expression ")"
    //              | INDENTIFIER
    //              ;
    //

    // Grammar notation Code representation:
    // Terminal     Code to match and consume a token
    // Nonterminal  Call to that rule’s function
    // |            if or switch statement
    // * or +       while or for loop
    // ?            if statement

    public record ParseExpressionResult(Expr? Expression, IReadOnlyList<ParseError> Errors)
    {
        public bool Success => Expression is not null && Errors.Count == 0;
    }

    public record ParseStatementsResult(IReadOnlyList<Stmt> Statements, IReadOnlyList<ParseError> Errors)
    {
        public bool Success => Statements.Count > 0 && Errors.Count == 0;
    }

    public class ParseError : Exception
    {
        public Token Token { get; }

        public ParseError(Token token, string message)
            : base(message)
        {
            Token = token;
        }

        public override string ToString()
        {
            return $"[{Token.Line}] Error at '{Token.Lexeme}': {Message}";
        }
    }

    private readonly IReadOnlyList<Token> _tokens;

    private readonly List<ParseError> _errors = [];

    private int _current = 0;

    public Parser(IReadOnlyList<Token> tokens)
    {
        _tokens = tokens;
    }

    public ParseExpressionResult ParseExpression()
    {
        try
        {
            var expr = Expression();

            // Check if all tokens were consumed.
            if (!IsAtEnd())
            {
                Error("Found extra tokens after expression.");
            }

            return new ParseExpressionResult(expr, _errors.ToArray());
        }
        catch (ParseError) // Some parse-errors are thrown, others are not.
        {
            return new ParseExpressionResult(null, _errors.ToArray());
        }
    }

    public ParseStatementsResult ParseStatements()
    {
        var statements = new List<Stmt>();

        while (!IsAtEnd())
        {
            var stmt = Declaration(); // Catches all parse-errors.
            if (stmt is null) continue;

            statements.Add(stmt);
        }

        return new ParseStatementsResult(statements.ToArray(), _errors.ToArray());
    }

    private Stmt? Declaration()
    {
        try
        {
            if (Match(TokenType.VAR))
                return VarDeclaration();

            return Statement();
        }
        catch (ParseError parseError)
        {
            _ = parseError;

            Synchronize();
            return null;
        }
    }

    private Stmt VarDeclaration()
    {
        Token name = Consume(TokenType.IDENTIFIER, "Expect variable name.");

        Expr? initializer = null;
        if (Match(TokenType.EQUAL))
        {
            initializer = Expression();
        }

        Consume(TokenType.SEMICOLON, "Expect ';' after variable declaration.");

        return new Stmt.Var(name, initializer);
    }

    private Stmt Statement()
    {
        if (Match(TokenType.IF)) return IfStatement();
        if (Match(TokenType.PRINT)) return PrintStatement();
        if (Match(TokenType.LEFT_BRACE)) return Block();

        return ExpressionStatement();
    }

    private Stmt IfStatement()
    {
        Consume(TokenType.LEFT_PAREN, "Expect '(' after 'if'.");
        Expr condition = Expression();
        Consume(TokenType.RIGHT_PAREN, "Expect ')' after if condition.");

        Stmt thenBranch = Statement();
        Stmt? elseBranch = null;
        if (Match(TokenType.ELSE))
        {
            elseBranch = Statement();
        }

        return new Stmt.If(condition, thenBranch, elseBranch);
    }

    private Stmt PrintStatement()
    {
        Expr expr = Expression();
        Consume(TokenType.SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Print(expr);
    }

    private Stmt ExpressionStatement()
    {
        Expr expr = Expression();
        Consume(TokenType.SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

    private Stmt Block()
    {
        return new Stmt.Block(BlockStatements());
    }

    private IReadOnlyList<Stmt> BlockStatements()
    {
        var statements = new List<Stmt>();

        while (!Check(TokenType.RIGHT_BRACE) && !IsAtEnd())
        {
            var stmt = Declaration();
            if (stmt is null) continue;
            statements.Add(stmt);
        }

        Consume(TokenType.RIGHT_BRACE, "Expect '}' after block.");

        return statements;
    }

    private Expr Expression()
    {
        return Assignment();
    }

    private Expr Assignment()
    {
        // Parse potential IDENTIFIER as if it were an ...-expression.
        Expr expr = LogicOr();

        // Assignment?
        if (Match(TokenType.EQUAL))
        {
            Token equals = Previous();
            Expr assignment = Assignment();

            // Check if l-value is valid assignment target.
            if (expr is Expr.Variable variableExpr)
            {
                return new Expr.Assign(variableExpr.Name, assignment);
            }
            else
            {
                _ = Error(equals, "Invalid assignment target."); // Not throwing
            }
        }

        return expr;
    }

    private Expr LogicOr()
    {
        Expr expr = LogicAnd();

        while (Match(TokenType.OR))
        {
            Token op = Previous();
            Expr right = LogicAnd();

            expr = new Expr.Logical(expr, op, right);
        }

        return expr;
    }

    private Expr LogicAnd()
    {
        Expr expr = Equality();

        while (Match(TokenType.AND))
        {
            Token op = Previous();
            Expr right = Equality();

            expr = new Expr.Logical(expr, op, right);
        }

        return expr;
    }

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

    private Expr Primary()
    {
        if (Match(TokenType.TRUE)) return new Expr.Literal(true);
        if (Match(TokenType.FALSE)) return new Expr.Literal(false);
        if (Match(TokenType.NIL)) return new Expr.Literal(null);

        if (Match(TokenType.NUMBER, TokenType.STRING))
        {
            return new Expr.Literal(Previous().Literal);
        }

        if (Match(TokenType.LEFT_PAREN))
        {
            Expr expr = Expression();
            Consume(TokenType.RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        if (Match(TokenType.IDENTIFIER))
        {
            return new Expr.Variable(Previous());
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

    private Exception Error(Token token, string message)
    {
        var error = new ParseError(token, message);
        _errors.Add(error);
        return error;
    }



    /// <summary>
    /// Recover from panic mode after encountering a parse error.
    /// </summary>
    private void Synchronize()
    {
        if (IsAtEnd())
            return;

        // Consume current token (which caused the error)
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
