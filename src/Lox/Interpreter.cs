using Lox.Model;

namespace Lox;

public struct Nothing { }

public class RuntimeError : Exception
{
    public Token Token { get; }

    public RuntimeError(Token token, string message)
        : base(message)
    {
        Token = token;
    }
}

public class Interpreter : Expr.IVisitor<object?>, Stmt.IVisitor<Nothing>
{
    private readonly Environment _globalEnvironment = new();


    public Interpreter()
    {
    }

    //public object? Interpret(Expr expr)
    //{
    //    try
    //    {
    //        return Evaluate(expr);
    //    }
    //    catch (RuntimeError runtimeError)
    //    {
    //        Console.WriteLine($"RuntimeError: {runtimeError.Message}");
    //        //_ = runtimeError;
    //        return null;
    //    }
    //}

    public void Interpret(IReadOnlyList<Stmt> statements)
    {
        try
        {
            foreach (var statement in statements)
            {
                Execute(statement);
            }
        }
        catch (RuntimeError runtimeError)
        {
            Console.WriteLine($"RuntimeError: {runtimeError.Message}");
        }
    }

    //
    // Statement visitor
    //

    public Nothing VisitExpressionStmt(Stmt.Expression expression)
    {
        _ = Evaluate(expression.Expr);
        return new Nothing();
    }

    public Nothing VisitPrintStmt(Stmt.Print print)
    {
        object? value = Evaluate(print.Expr);
        Console.WriteLine($"{value}"); // TODO stringify
        return new Nothing();
    }

    public Nothing VisitVarStmt(Stmt.Var var)
    {
        object? value = var.Initializer is not null
            ? Evaluate(var.Initializer)
            : null;

        _globalEnvironment.Define(var.Name.Lexeme, value);
        return new Nothing();
    }

    //
    // Expression visitor
    //

    public object? VisitAssignExpr(Expr.Assign assign)
    {
        object? value = Evaluate(assign.Value);
        _globalEnvironment.Assign(assign.Name.Lexeme, value, assign.Name);
        return value;
    }

    public object? VisitLiteralExpr(Expr.Literal literal)
    {
        return literal.Value;
    }

    public object? VisitGroupingExpr(Expr.Grouping grouping)
    {
        return Evaluate(grouping.Expression);
    }

    public object? VisitUnaryExpr(Expr.Unary unary)
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

    public object? VisitBinaryExpr(Expr.Binary binary)
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

    public object? VisitVariableExpr(Expr.Variable variable)
    {
        return _globalEnvironment.Get(variable.Name.Lexeme, variable.Name);
    }



    //
    // Utils
    //

    private void Execute(Stmt stmt)
    {
        _ = stmt.Accept(this);
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
