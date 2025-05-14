using Lox.Model;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;

namespace Lox;

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
        private readonly string? _name;
        private readonly Expr.Function _function;
        private readonly Environment _closure;
        private readonly bool _isInitMethod; // TODO i don't like this

        public int Arity { get; }

        public LoxFunction(string? name, Expr.Function function, Environment closure, bool isInitMethod)
        {
            _name = name;
            _function = function;
            _closure = closure;
            _isInitMethod = isInitMethod; // TODO i don't like this
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
                // TODO i don't like this
                if (_isInitMethod)
                {
                    // init() methods always return 'this';
                    return _closure.GetAtHop("this", 0);
                }

                return returnException.Value;
            }

            // TODO i don't like this
            if (_isInitMethod)
            {
                // init() methods always return 'this';
                return _closure.GetAtHop("this", 0);
            }

            return null;
        }

        public LoxFunction Bind(LoxInstance instance) // TODO maybe move to LoxInstance ?
        {
            var env = new Environment(_closure);
            env.Define("this", instance);
            return new LoxFunction(_name, _function, env, _isInitMethod);
        }

        public override string ToString()
        {
            return _name is not null ? $"<fn {_name}>" : "<fn>";
        }
    }

    private class LoxClass : ILoxCallable
    {
        private readonly IReadOnlyDictionary<string, LoxFunction> _methods;
        private readonly LoxFunction? _initMethod;

        public string Name { get; }

        public LoxClass(string name, IReadOnlyDictionary<string, LoxFunction> methods)
        {
            Name = name;
            _methods = methods;

            if (methods.TryGetValue("init", out var initMethod))
            {
                _initMethod = initMethod;
                Arity = initMethod.Arity;
            }
            else
            {
                _initMethod = null;
                Arity = 0;
            }
        }

        public int Arity { get; }

        public object? Call(Interpreter interpreter, IReadOnlyList<object?> args)
        {
            var instance = new LoxInstance(this);

            if (_initMethod != null)
            {
                _initMethod.Bind(instance).Call(interpreter, args);
            }

            return instance;
        }

        public bool FindMethod(string name, [NotNullWhen(true)] out LoxFunction? method)
        {
            if (_methods.TryGetValue(name, out var m))
            {
                method = m;
                return true;
            }

            // TODO inheritance etc

            method = null;
            return false;
        }

        public override string ToString()
        {
            return Name; // that's what the tests want.
            //return $"<class {Name}>";
        }
    }

    private class LoxInstance
    {
        private readonly LoxClass _class;
        private readonly Dictionary<string, object?> _fields = [];

        public LoxInstance(LoxClass @class)
        {
            _class = @class;
        }

        public object? Get(string name, Token tokenForError)
        {
            // Terminology: return a property which is either a field or a method.

            if (_fields.TryGetValue(name, out var value))
                return value;

            if (_class.FindMethod(name, out var method))
                return method.Bind(this);

            throw new RuntimeError(tokenForError, $"Undefined property '{name}'.");
        }

        public void Set(string name, object? value, Token tokenForError)
        {
            _ = tokenForError;

            _fields[name] = value;
        }

        public override string ToString()
        {
            return $"{_class.Name} instance"; // that's what the tests want.
            //return $"<instance of {_class}>";
        }
    }

    private readonly Environment _globalEnvironment = new();
    private Environment _environment;
    private readonly DateTime _startTime;

    public Interpreter()
    {
        _environment = _globalEnvironment;
        _startTime = DateTime.Now;

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

        // xxx sneaky debug tools xxx
        switch (s)
        {
            case "env":
                _environment.Dump();
                break;
        }
        // xxx

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
                        // Must be executed on block or else 'HopsToEnv' is off by one.
                        ExecuteBlock([ blockStatement.Statements[1] ], new Environment(_environment));
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
        var loxFunction = new LoxFunction(function.Name.Lexeme, function.Fun, _environment, false);
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

    public Nothing VisitClassStmt(Stmt.Class @class)
    {
        _environment.Define(@class.Name.Lexeme, null); // TODO is this actually required?

        var methods = @class.Methods
            .Select(m => (m.Name.Lexeme, Method: new LoxFunction(m.Name.Lexeme, m.Fun, _environment, m.Name.Lexeme == "init")))
            .ToDictionary(x => x.Lexeme, x => x.Method);

        var loxClass = new LoxClass(@class.Name.Lexeme, methods);

        _environment.AssignAtHop(@class.Name.Lexeme, loxClass, 0);

        return new Nothing();
    }

    public Nothing VisitMethodStmt(Stmt.Method method)
    {
        throw new InvalidOperationException("Must not reach");
    }

    //
    // Expression visitor
    //

    public object? VisitAssignExpr(Expr.Assign assign)
    {
        if (!assign.HopsToEnv.HasValue)
            throw new InvalidOperationException("Internal error. HopsToEnv not set on Expr.Assign.");

        object? value = Evaluate(assign.Value);

        _environment.AssignAtHop(assign.Name.Lexeme, value, assign.HopsToEnv.Value);

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
        if (!variable.HopsToEnv.HasValue)
            throw new InvalidOperationException("Internal error. HopsToEnv not set on Expr.Variable.");

        return _environment.GetAtHop(variable.Name.Lexeme, variable.HopsToEnv.Value);
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

    public object? VisitFunctionExpr(Expr.Function function)
    {
        var loxFunction = new LoxFunction(null, function, _environment, false);
        return loxFunction;
    }

    public object? VisitGetExpr(Expr.Get get)
    {
        object? obj = Evaluate(get.Object);

        if (obj is LoxInstance instance)
        {
            return instance.Get(get.Name.Lexeme, get.Name);
        }

        throw new RuntimeError(get.Name, "Only instances have properties.");
    }

    public object? VisitSetExpr(Expr.Set set)
    {
        object? obj = Evaluate(set.Object);
        object? value = Evaluate(set.Value);

        if (obj is LoxInstance instance)
        {
            instance.Set(set.Name.Lexeme, value, set.Name);
            return value;
        }

        throw new RuntimeError(set.Name, "Only instances have properties.");
    }

    public object? VisitThisExpr(Expr.This @this)
    {
        if (!@this.HopsToEnv.HasValue)
            throw new InvalidOperationException("Internal error. HopsToEnv not set on Expr.This.");

        return _environment.GetAtHop(@this.Token.Lexeme, @this.HopsToEnv.Value);
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
