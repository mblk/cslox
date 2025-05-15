using System.Globalization;
using System.Text;

namespace Lox.Model;

public enum TokenType
{
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // Literals.
    IDENTIFIER, STRING, NUMBER,

    // Keywords.
    AND, BREAK, CLASS, CONTINUE, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    EOF
}

public record Token(TokenType Type, string Lexeme, object? Literal, int Line)
{
    public override string ToString()
    {
        var sb = new StringBuilder();

        sb.Append('[');
        sb.Append(Line);
        sb.Append(']');
        sb.Append(' ');

        sb.Append(Type);
        sb.Append(' ');
        sb.Append(Lexeme);
        sb.Append(' ');

        sb.Append(Literal switch
        {
            null => "null",
            bool b => b ? "true" : "false",
            double d => d.ToString(CultureInfo.InvariantCulture),
            _ => Literal.ToString(),
        });

        return sb.ToString();
    }
}
