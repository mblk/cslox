using Lox.Model;
using System.Diagnostics;
using System.Globalization;

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

    public override string ToString()
    {
        return $"RuntimeError: {Message}";
    }
}

public interface ILoxCallable
{
    int Arity { get; }

    object? Call(Interpreter interpreter, IReadOnlyList<object?> args);
}

public class Interpreter : Expr.IVisitor<object?>, Stmt.IVisitor<Nothing>
{
    private class ControlException : Exception
    {
        public Token Token { get; } // break/continue

        public ControlException(Token token)
        {
            Token = token;
        }
    }

    private class ReturnException : Exception
    {
        public object? Value { get; }

        public ReturnException(object? value)
        {
            Value = value;
        }
    }

    private class NativeFunction : ILoxCallable
    {
        private readonly string _name;
        private readonly Func<IReadOnlyList<object?>, object?> _func;

        public int Arity { get; }

        public NativeFunction(string name, int arity, Func<IReadOnlyList<object?>, object?> func)
        {
            _name = name;
            Arity = arity;
            _func = func;
        }

        public object? Call(Interpreter interpreter, IReadOnlyList<object?> args)
        {
            return _func(args);
        }

        public override string ToString()
        {
            return $"<native fn {_name}>";
        }
    }

    private class LoxFunction : ILoxCallable
    {
        private readonly Stmt.Function _function;
        private readonly Environment _closure;

        public int Arity { get; }

        public LoxFunction(Stmt.Function function, Environment closure)
        {
            _function = function;
            _closure = closure;
            Arity = _function.Parms.Count;
        }

        public object? Call(Interpreter interpreter, IReadOnlyList<object?> args)
        {
            Debug.Assert(_function.Parms.Count == args.Count);

            var environment = new Environment(_closure);

            for (var i = 0; i<_function.Parms.Count; i++)
            {
                environment.Define(_function.Parms[i].Lexeme, args[i]);
            }

            try
            {
                interpreter.ExecuteBlock(_function.Body, environment);
            }
            catch (ReturnException returnException)
            {
                return returnException.Value;
            }

            return null;
        }

        public override string ToString()
        {
            return $"<fn {_function.Name.Lexeme}>";
        }
    }

    private readonly Environment _globalEnvironment = new();
    private Environment _environment;
    private readonly DateTime _startTime;

    public Interpreter()
    {
        _environment = _globalEnvironment;
        _startTime = Process.GetCurrentProcess().StartTime;

        _globalEnvironment.Define("clock", new NativeFunction("clock", 0, (args) =>
        {
            return (DateTime.Now - _startTime).TotalSeconds;
        }));
    }

    public object? Interpret(Expr expr)
    {
        try
        {
            return Evaluate(expr);
        }
        catch (RuntimeError runtimeError)
        {
            Console.WriteLine(runtimeError);
            return null;
        }
    }

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
            Console.WriteLine(runtimeError);
        }
        catch (ControlException controlException)
        {
            Console.WriteLine($"Unhandled control exception: {controlException.Token.Lexeme}");
        }
    }

    //
    // Statement visitor
    //

    public Nothing VisitBlockStmt(Stmt.Block block)
    {
        ExecuteBlock(block.Statements, new Environment(_environment));
        return new Nothing();
    }

    public Nothing VisitExpressionStmt(Stmt.Expression expression)
    {
        _ = Evaluate(expression.Expr);
        return new Nothing();
    }

    public Nothing VisitPrintStmt(Stmt.Print print)
    {
        object? value = Evaluate(print.Expr);
        var s = Stringify(value);
        Console.WriteLine(s);
        return new Nothing();
    }

    public Nothing VisitVarStmt(Stmt.Var var)
    {
        object? value = var.Initializer is not null
            ? Evaluate(var.Initializer)
            : null;

        _environment.Define(var.Name.Lexeme, value);
        return new Nothing();
    }

    public Nothing VisitIfStmt(Stmt.If @if)
    {
        if (IsTruthy(Evaluate(@if.Condition)))
        {
            Execute(@if.ThenBranch);
        }
        else if (@if.ElseBranch is not null)
        {
            Execute(@if.ElseBranch);
        }
        return new Nothing();
    }

    public Nothing VisitWhileStmt(Stmt.While @while)
    {
        while (IsTruthy(Evaluate(@while.Condition)))
        {
            try
            {
                Execute(@while.Body);
            }
            catch (ControlException controlException)
            {
                if (controlException.Token.Type == TokenType.BREAK)
                {
                    break;
                }
                else if (controlException.Token.Type == TokenType.CONTINUE)
                {
                    // Must execute the increment-expression of for-loops.
                    if (@while.Body is Stmt.Block blockStatement &&
                        blockStatement.Statements.Count == 2 &&
                        blockStatement.Statements[1].Tag == "for_increment")
                    {
                        Execute(blockStatement.Statements[1]);
                    }

                    continue;
                }
                else
                {
                    throw new NotImplementedException("Invalid control exception");
                }
            }
        }
        return new Nothing();
    }

    public Nothing VisitControlStmt(Stmt.Control control)
    {
        throw new ControlException(control.Op);
    }

    public Nothing VisitFunctionStmt(Stmt.Function function)
    {
        var loxFunction = new LoxFunction(function, _environment);
        _environment.Define(function.Name.Lexeme, loxFunction);
        return new Nothing();
    }

    public Nothing VisitReturnStmt(Stmt.Return @return)
    {
        object? value = @return.Expr is not null
            ? Evaluate(@return.Expr)
            : null;

        throw new ReturnException(value);
    }

    //
    // Expression visitor
    //

    public object? VisitAssignExpr(Expr.Assign assign)
    {
        object? value = Evaluate(assign.Value);
        _environment.Assign(assign.Name.Lexeme, value, assign.Name);
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

            throw new RuntimeError(binary.Op, "Operands must be two numbers or two strings.");
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

    public object? VisitLogicalExpr(Expr.Logical logical)
    {
        object? leftValue = Evaluate(logical.Left);
        bool leftThruthy = IsTruthy(leftValue);

        // short circuit?
        switch (logical.Op.Type)
        {
            case TokenType.AND:
                if (!leftThruthy)
                    return leftValue;
                break;

            case TokenType.OR:
                if (leftThruthy)
                    return leftValue;
                break;

            default:
                throw new NotImplementedException("Invalid op in VisitLogicalExpr");
        }

        return Evaluate(logical.Right);
    }

    public object? VisitVariableExpr(Expr.Variable variable)
    {
        return _environment.Get(variable.Name.Lexeme, variable.Name);
    }



    public object? VisitCallExpr(Expr.Call call)
    {
        object? callee = Evaluate(call.Callee);

        var args = call.Arguments
            .Select(Evaluate)
            .ToArray();

        if (callee is ILoxCallable callable)
        {
            if (callable.Arity != args.Length)
            {
                throw new RuntimeError(call.Paren, $"Expected {callable.Arity} arguments but got {args.Length}.");
            }

            return callable.Call(this, args);
        }
        else
        {
            throw new RuntimeError(call.Paren, "Can only call functions and classes.");
        }
    }





    //
    // Utils
    //

    private void Execute(Stmt stmt)
    {
        _ = stmt.Accept(this);
    }

    private void ExecuteBlock(IReadOnlyList<Stmt> statements, Environment environment)
    {
        var prevEnvironment = _environment;
        try
        {
            _environment = environment;

            foreach (var statement in statements)
            {
                Execute(statement);
            }
        }
        finally
        {
            _environment = prevEnvironment;
        }
    }

    private object? Evaluate(Expr expr)
    {
        return expr.Accept(this);
    }

    private static string Stringify(object? value) // TODO move to extension/helper class?
    {
        if (value is null) return "nil";
        if (value is bool boolValue) return boolValue ? "true" : "false";

        if (value is double doubleValue)
        {
            return doubleValue.ToString(CultureInfo.InvariantCulture);
        }

        return value.ToString() ?? "???";
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

        if (a is double doubleA && b is double doubleB)
        {
            return doubleA == doubleB;
        }

        return a.Equals(b);
    }

}
