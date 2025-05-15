﻿using Lox.Model;

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

    private enum CurrentClass
    {
        None,
        Class,
        SubClass,
    }

    private enum CurrentFunction
    {
        None,
        Lambda,
        Function,
        Method,
        Initializer,
    }

    private enum CurrentLoop
    {
        None,
        While,
    }

    private readonly List<ResolveError> _errors = [];
    private readonly Stack<Dictionary<string, bool>> _scopes = [];

    private CurrentClass _currentClass = CurrentClass.None;
    private CurrentFunction _currentFunction = CurrentFunction.None;
    private CurrentLoop _currentLoop = CurrentLoop.None;

    public Resolver()
    {
    }

    public ResolverResult DoWork(IReadOnlyList<Stmt> statements)
    {
        BeginScope(); // implicit global scope

        DeclareAndDefine("clock", null);

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

        ResolveFunctionExpression(function.Fun, CurrentFunction.Function);

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

        if (_currentFunction == CurrentFunction.Initializer && @return.Expr is not null)
        {
            Error(@return.Token, "Can't return a value from an initializer.");
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
        {
            Resolve(@while.Body);
        }
        _currentLoop = prevLoop;

        return new Nothing();
    }

    public Nothing VisitClassStmt(Stmt.Class @class)
    {
        var prevClass = _currentClass;
        _currentClass = CurrentClass.Class;
        {
            Declare(@class.Name.Lexeme, @class.Name);
            Define(@class.Name.Lexeme);

            if (@class.Superclass is not null)
            {
                _currentClass = CurrentClass.SubClass;

                if (@class.Name.Lexeme == @class.Superclass.Name.Lexeme)
                {
                    Error(@class.Superclass.Name, "A class can't inherit from itself.");
                }
                else
                {
                    Resolve(@class.Superclass);
                }

                BeginScope(); // This is the scope that contains the 'super' definition (created at runtime when a class is declared).
                DeclareAndDefine("super", @class.Superclass.Name);
            }

            BeginScope(); // This is the scope that methods are bound to (created at runtime when a method is accessed).
            {
                DeclareAndDefine("this", @class.Name);

                foreach (var method in @class.Methods)
                {
                    Resolve(method);
                }
            }
            EndScope();

            if (@class.Superclass is not null)
            {
                EndScope();
            }
        }
        _currentClass = prevClass;

        return new Nothing();
    }

    public Nothing VisitMethodStmt(Stmt.Method method)
    {
        // Not adding the name to the current scope.
        // Commented out code left here because i'm not sure if this is required or not.
        //Declare(method.Name.Lexeme, method.Name);
        //Define(method.Name.Lexeme);

        ResolveFunctionExpression(method.Fun, method.Name.Lexeme == "init" ? CurrentFunction.Initializer : CurrentFunction.Method);

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
        ResolveFunctionExpression(function, CurrentFunction.Lambda);

        return new Nothing();
    }

    public Nothing VisitGetExpr(Expr.Get get)
    {
        Resolve(get.Object);

        return new Nothing();
    }

    public Nothing VisitSetExpr(Expr.Set set)
    {
        Resolve(set.Object);
        Resolve(set.Value);

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

    public Nothing VisitThisExpr(Expr.This @this)
    {
        if (_currentClass == CurrentClass.None)
        {
            Error(@this.Token, "Can't use 'this' outside of a class.");
            return new Nothing();
        }

        if (FindInScopes("this", out var hops))
        {
            SaveResolution(@this, hops);
        }
        else
        {
            throw new InvalidOperationException("Internal error. This not found in surrounding scopes.");
        }

        return new Nothing();
    }

    public Nothing VisitSuperExpr(Expr.Super super)
    {
        if (_currentClass == CurrentClass.None)
        {
            Error(super.Token, "Can't use 'super' outside of a class.");
            return new Nothing();
        }
        else if (_currentClass != CurrentClass.SubClass)
        {
            Error(super.Token, "Can't use 'super' in a class with no superclass.");
            return new Nothing();
        }

        if (FindInScopes("super", out var hops))
        {
            SaveResolution(super, hops);
        }
        else
        {
            throw new InvalidOperationException("Internal error. Super not found in surrounding scopes.");
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

    private void DeclareAndDefine(string name, Token? token)
    {
        Declare(name, token);
        Define(name);
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

    private void DumpScopes()
    {
        Console.WriteLine($"== DumpScopes ==");

        for (var i = 0; i<_scopes.Count; i++)
        {
            var scope = _scopes.ElementAt(i);

            foreach (var (key, value) in scope)
            {
                Console.WriteLine($"[{i}] {key} -> {value}");
            }
        }
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

    private void ResolveFunctionExpression(Expr.Function function, CurrentFunction functionType)
    {
        BeginScope();
        {
            foreach (var parm in function.Parms)
            {
                Declare(parm.Lexeme, parm);
                Define(parm.Lexeme);
            }

            var prevFunction = _currentFunction;
            _currentFunction = functionType;
            {
                Resolve(function.Body);
            }
            _currentFunction = prevFunction;
        }
        EndScope();
    }

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
