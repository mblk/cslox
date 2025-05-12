using Lox.Model;

namespace Lox;

public class Resolver : Expr.IVisitor<Nothing>, Stmt.IVisitor<Nothing>
{
    public record ResolverResult(IReadOnlyList<ResolveError> Errors)
    {
        public bool Success => Errors.Count == 0;
    }
    public record ResolveError(Token? Token, string Message)
    {
        public override string ToString()
        {
            if (Token is null)
            {
                return $"ResolveError: {Message}";
            }
            else
            {
                return $"[{Token.Line}] ResolveError at '{Token.Lexeme}': {Message}";
            }
        }
    }

    private enum CurrentFunction
    {
        None,
        Function,
        Method,
    }

    private enum CurrentLoop
    {
        None,
        While,
    }

    private readonly List<ResolveError> _errors = [];
    private readonly Stack<Dictionary<string, bool>> _scopes = [];

    private CurrentFunction _currentFunction = CurrentFunction.None;
    private CurrentLoop _currentLoop = CurrentLoop.None;

    public Resolver()
    {
    }

    public ResolverResult DoWork(IReadOnlyList<Stmt> statements)
    {
        BeginScope(); // implicit global scope

        Declare("clock", null);
        Define("clock");

        Resolve(statements);
        EndScope();

        return new ResolverResult(_errors.ToArray());
    }

    //
    // Statements visitor
    //

    public Nothing VisitBlockStmt(Stmt.Block block)
    {
        BeginScope();
        Resolve(block.Statements);
        EndScope();

        return new Nothing();
    }

    public Nothing VisitControlStmt(Stmt.Control control)
    {
        if (_currentLoop == CurrentLoop.None)
        {
            Error(control.Op, $"Can't {control.Op.Lexeme} outside loop.");
        }

        return new Nothing();
    }

    public Nothing VisitExpressionStmt(Stmt.Expression expression)
    {
        Resolve(expression.Expr);

        return new Nothing();
    }

    public Nothing VisitFunctionStmt(Stmt.Function function)
    {
        Declare(function.Name.Lexeme, function.Name);
        Define(function.Name.Lexeme);

        Resolve(function.Fun);

        return new Nothing();
    }

    public Nothing VisitIfStmt(Stmt.If @if)
    {
        Resolve(@if.Condition);
        Resolve(@if.ThenBranch);
        if (@if.ElseBranch is not null)
            Resolve(@if.ElseBranch);

        return new Nothing();
    }

    public Nothing VisitPrintStmt(Stmt.Print print)
    {
        Resolve(print.Expr);

        return new Nothing();
    }

    public Nothing VisitReturnStmt(Stmt.Return @return)
    {
        if (_currentFunction == CurrentFunction.None)
        {
            Error(@return.Token, "Can't return from top-level code.");
        }

        if (@return.Expr is not null)
            Resolve(@return.Expr);

        return new Nothing();
    }

    public Nothing VisitVarStmt(Stmt.Var var)
    {
        Declare(var.Name.Lexeme, var.Name);

        if (var.Initializer is not null)
            Resolve(var.Initializer);

        Define(var.Name.Lexeme);

        return new Nothing();
    }

    public Nothing VisitWhileStmt(Stmt.While @while)
    {
        Resolve(@while.Condition);

        var prevLoop = _currentLoop;
        _currentLoop = CurrentLoop.While;

        Resolve(@while.Body);
        
        _currentLoop = prevLoop;

        return new Nothing();
    }

    //
    // Expression visitor
    //

    public Nothing VisitAssignExpr(Expr.Assign assign)
    {
        Resolve(assign.Value);

        if (FindInScopes(assign.Name.Lexeme, out var hops))
        {
            SaveResolution(assign, hops);
        }
        else
        {
            Error(assign.Name, $"Undefined variable '{assign.Name.Lexeme}'.");
        }

        return new Nothing();
    }

    public Nothing VisitBinaryExpr(Expr.Binary binary)
    {
        Resolve(binary.Left);
        Resolve(binary.Right);

        return new Nothing();
    }

    public Nothing VisitCallExpr(Expr.Call call)
    {
        Resolve(call.Callee);

        foreach (var arg in call.Arguments)
            Resolve(arg);

        return new Nothing();
    }

    public Nothing VisitFunctionExpr(Expr.Function function)
    {
        BeginScope();

        foreach (var parm in function.Parms)
        {
            Declare(parm.Lexeme, parm);
            Define(parm.Lexeme);
        }

        var prevFunction = _currentFunction;
        _currentFunction = CurrentFunction.Function;

        Resolve(function.Body);
        
        _currentFunction = prevFunction;

        EndScope();

        return new Nothing();
    }

    public Nothing VisitGroupingExpr(Expr.Grouping grouping)
    {
        Resolve(grouping.Expression);

        return new Nothing();
    }

    public Nothing VisitLiteralExpr(Expr.Literal literal)
    {
        return new Nothing();
    }

    public Nothing VisitLogicalExpr(Expr.Logical logical)
    {
        Resolve(logical.Left);
        Resolve(logical.Right);

        return new Nothing();
    }

    public Nothing VisitUnaryExpr(Expr.Unary unary)
    {
        Resolve(unary.Right);

        return new Nothing();
    }

    public Nothing VisitVariableExpr(Expr.Variable variable)
    {
        // var a = a; ?
        if (IsInInitializer(variable.Name))
        {
            Error(variable.Name, $"Can't read local variable in its own initializer.");
            return new Nothing();
        }

        if (FindInScopes(variable.Name.Lexeme, out var hops))
        {
            SaveResolution(variable, hops);
        }
        else
        {
            Error(variable.Name, $"Undefined variable '{variable.Name.Lexeme}'.");
        }

        return new Nothing();
    }

    //
    // Scope management
    //

    // Must be kept in sync with the interpreter:
    // - Whenever a new environment is created in the interpreter, a matching scope is required in the resolver.
    private void BeginScope()
    {
        _scopes.Push([]);
    }

    private void EndScope()
    {
        _ = _scopes.Pop();
    }

    private void Declare(string name, Token? token)
    {
        // global scope?
        if (_scopes.Count == 0)
            return;

        var scope = _scopes.Peek();

        if (scope.ContainsKey(name))
        {
            Error(token, "Already a variable with this name in this scope.");
            return;
        }

        scope.Add(name, false); // false = declared but not defined
    }

    private void Define(string name)
    {
        // global scope?
        if (_scopes.Count == 0)
            return;

        var scope = _scopes.Peek();

        if (!scope.ContainsKey(name))
            throw new InvalidOperationException("Internal error. Name to define not in scope.");

        scope[name] = true; // true = declared and defined
    }

    private bool IsInInitializer(Token name)
    {
        for (var i = 0; i < _scopes.Count; i++)
        {
            var scope = _scopes.ElementAt(i); // 0 = top (what peek returns)

            if (scope.TryGetValue(name.Lexeme, out var isDefined))
            {
                if (isDefined)
                {
                    // all good.
                    return false;
                }
                else
                {
                    // currently resolving the initializer.
                    return true;
                }
            }
            else
            {
                // continue with next scope.
            }
        }

        // all good.
        return false;
    }

    private bool FindInScopes(string name, out int resolution)
    {
        for (var hops = 0; hops < _scopes.Count; hops++)
        {
            var scope = _scopes.ElementAt(hops); // 0 = top (what peek returns)

            if (scope.ContainsKey(name))
            {
                resolution = hops;
                return true;
            }
        }

        resolution = 0;
        return false;
    }

    private void SaveResolution(Expr expr, int hops)
    {
        if (expr.HopsToEnv is not null)
            throw new InvalidOperationException("Internal error. HopsToEnv already set on expr.");

        expr.HopsToEnv = hops;
    }

    //
    // Utils
    //

    private void Resolve(IEnumerable<Stmt> block)
    {
        foreach (var stmt in block)
        {
            Resolve(stmt);
        }
    }

    private void Resolve(Stmt stmt)
    {
        stmt.Accept(this);
    }

    private void Resolve(Expr expr)
    {
        expr.Accept(this);
    }

    private void Error(Token? token, string message)
    {
        _errors.Add(new ResolveError(token, message));
    }
}
