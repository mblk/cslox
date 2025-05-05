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


}