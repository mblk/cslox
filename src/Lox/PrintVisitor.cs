using Lox.Model;
using System.Text;

namespace Lox;

public class PrintVisitor : Expr.IVisitor<string>, Stmt.IVisitor<string>
{
    public string VisitAssignExpr(Expr.Assign assign)
    {
        return FormatAndAcceptChilds($"Assign {assign.Name.Lexeme}", assign.Value);
    }

    public string VisitBinaryExpr(Expr.Binary binary)
    {
        return FormatAndAcceptChilds($"Binary {binary.Op.Lexeme}", binary.Left, binary.Right);
    }

    public string VisitLogicalExpr(Expr.Logical logical)
    {
        return FormatAndAcceptChilds($"Logical {logical.Op.Lexeme}", logical.Left, logical.Right);
    }

    public string VisitGroupingExpr(Expr.Grouping grouping)
    {
        return FormatAndAcceptChilds($"Grouping", grouping.Expression);
    }

    public string VisitLiteralExpr(Expr.Literal literal)
    {
        return $"Literal {literal.Value ?? "nil"}";
    }

    public string VisitUnaryExpr(Expr.Unary unary)
    {
        return FormatAndAcceptChilds($"Unary {unary.Op.Lexeme}", unary.Right);
    }

    public string VisitVariableExpr(Expr.Variable variable)
    {
        return $"Variable {variable.Name.Lexeme}";
    }

    public string VisitCallExpr(Expr.Call call)
    {
        return $"Call {call.Callee.Accept(this)}({String.Join(", ", call.Arguments.Select(x => x.Accept(this)))})";
    }

    public string VisitGetExpr(Expr.Get get)
    {
        return $"Get {get.Object.Accept(this)} . {get.Name.Lexeme}";
    }

    public string VisitSetExpr(Expr.Set set)
    {
        return $"Set {set.Object.Accept(this)} . {set.Name.Lexeme} = {set.Value.Accept(this)}";
    }

    public string VisitFunctionExpr(Expr.Function function)
    {
        var sb = new StringBuilder();

        sb.AppendLine($"Function ({String.Join(", ", function.Parms.Select(p => p.Lexeme))}) {{");
        foreach (var stmt in function.Body)
        {
            sb.Append("    "); // TODO must indent every line
            sb.AppendLine(stmt.Accept(this));
        }
        sb.AppendLine("}");

        return sb.ToString();
    }

    public string VisitThisExpr(Expr.This @this)
    {
        return $"This";
    }

    public string VisitSuperExpr(Expr.Super super)
    {
        return "Super";
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




    public string VisitBlockStmt(Stmt.Block block)
    {
        var sb = new StringBuilder();

        sb.Append("Block { ");

        foreach (var stmt in block.Statements)
        {
            sb.Append(stmt.Accept(this));
        }

        sb.Append("} ");

        return sb.ToString();
    }

    public string VisitExpressionStmt(Stmt.Expression expression)
    {
        return $"{expression.Expr.Accept(this)}; ";
    }

    public string VisitPrintStmt(Stmt.Print print)
    {
        return $"Print {print.Expr.Accept(this)}; ";
    }

    public string VisitVarStmt(Stmt.Var var)
    {
        if (var.Initializer is null)
        {
            return $"Var {var.Name.Lexeme}; ";
        }
        else
        {
            return $"Var {var.Name.Lexeme} = {var.Initializer.Accept(this)}; ";
        }
    }

    public string VisitIfStmt(Stmt.If @if)
    {
        var cond = @if.Condition.Accept(this);
        var then = @if.ThenBranch.Accept(this);
        var @else = @if.ElseBranch != null ? @if.ElseBranch.Accept(this) : "---";

        return $"If ({cond}) {then} else {@else} ";
    }

    public string VisitWhileStmt(Stmt.While @while)
    {
        var cont = @while.Condition.Accept(this);
        var body = @while.Body.Accept(this);

        return $"While ({cont}) {body} ";
    }

    public string VisitControlStmt(Stmt.Control control)
    {
        return $"Control {control.Op.Lexeme}";
    }

    public string VisitFunctionStmt(Stmt.Function function)
    {
        var sb = new StringBuilder();

        sb.AppendLine($"Function {function.Name.Lexeme} ({String.Join(", ", function.Fun.Parms.Select(p => p.Lexeme))}) {{"); // TODO reuse fun-expr printer
        foreach (var stmt in function.Fun.Body)
        {
            sb.Append("    "); // TODO must indent every line
            sb.AppendLine(stmt.Accept(this));
        }
        sb.AppendLine("}");

        return sb.ToString();
    }

    public string VisitReturnStmt(Stmt.Return @return)
    {
        if (@return.Expr is null)
            return "Return; ";

        return $"Return {@return.Expr.Accept(this)}; ";
    }

    public string VisitClassStmt(Stmt.Class @class)
    {
        var sb = new StringBuilder();

        sb.AppendLine($"Class {@class.Name.Lexeme}");
        sb.AppendLine("{");

        foreach (var m in @class.Methods)
        {
            sb.AppendLine(m.Accept(this));
        }

        sb.AppendLine("}");

        return sb.ToString();
    }

    public string VisitMethodStmt(Stmt.Method method)
    {
        var sb = new StringBuilder();

        sb.AppendLine($"Method {method.Name.Lexeme} ({String.Join(", ", method.Fun.Parms.Select(p => p.Lexeme))}) {{"); // TODO reuse fun-expr printer
        foreach (var stmt in method.Fun.Body)
        {
            sb.Append("    "); // TODO must indent every line
            sb.AppendLine(stmt.Accept(this));
        }
        sb.AppendLine("}");

        return sb.ToString();
    }


}