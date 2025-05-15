using Lox.Model;
using System.Collections.Frozen;
using System.Globalization;

namespace Lox;

public class Scanner
{
    public record ScanResult(IReadOnlyList<Token> Tokens, IReadOnlyList<ScanError> Errors)
    {
        public bool Success => Tokens.Count > 0 && Errors.Count == 0;
    }

    public record ScanError(int Line, string Message)
    {
        public override string ToString()
        {
            return $"[{Line}] Error: {Message}";
        }
    }

    private readonly FrozenDictionary<char, TokenType> _singleCharacterTokens
        = new Dictionary<char, TokenType>()
        {
            { '(', TokenType.LEFT_PAREN },
            { ')', TokenType.RIGHT_PAREN },
            { '{', TokenType.LEFT_BRACE },
            { '}', TokenType.RIGHT_BRACE },
            { ',', TokenType.COMMA },
            { '.', TokenType.DOT },
            { '-', TokenType.MINUS },
            { '+', TokenType.PLUS },
            { ';', TokenType.SEMICOLON },
            { '*', TokenType.STAR },
        }
        .ToFrozenDictionary();

    private readonly FrozenDictionary<char, (TokenType, char, TokenType)> _oneOrTwoCharacterTokens
        = new Dictionary<char, (TokenType, char, TokenType)>()
        {
            { '!', ( TokenType.BANG,    '=', TokenType.BANG_EQUAL    ) },
            { '=', ( TokenType.EQUAL,   '=', TokenType.EQUAL_EQUAL   ) },
            { '<', ( TokenType.LESS,    '=', TokenType.LESS_EQUAL    ) },
            { '>', ( TokenType.GREATER, '=', TokenType.GREATER_EQUAL ) },
        }
        .ToFrozenDictionary();

    private readonly FrozenDictionary<string, TokenType> _keywords
        = new Dictionary<string, TokenType>()
        {
            { "and",      TokenType.AND },
            { "break",    TokenType.BREAK },
            { "class",    TokenType.CLASS },
            { "continue", TokenType.CONTINUE },
            { "else",     TokenType.ELSE },
            { "false",    TokenType.FALSE },
            { "fun",      TokenType.FUN },
            { "for",      TokenType.FOR },
            { "if",       TokenType.IF },
            { "nil",      TokenType.NIL },
            { "or",       TokenType.OR },
            { "print",    TokenType.PRINT },
            { "return",   TokenType.RETURN },
            { "super",    TokenType.SUPER },
            { "this",     TokenType.THIS },
            { "true",     TokenType.TRUE },
            { "var",      TokenType.VAR },
            { "while",    TokenType.WHILE },
        }
        .ToFrozenDictionary();

    private readonly string _source;

    private readonly List<Token> _tokens = [];
    private readonly List<ScanError> _errors = [];

    private int _start = 0;
    private int _current = 0;
    private int _line = 1;

    public Scanner(string source)
    {
        _source = source;
    }

    public ScanResult ScanTokens()
    {
        while (!IsAtEnd())
        {
            ScanToken();
        }

        _tokens.Add(new Token(TokenType.EOF, String.Empty, null, _line));

        return new ScanResult(_tokens.ToArray(), _errors.ToArray());
    }

    private void ScanToken()
    {
        _start = _current;

        var c = Advance();
        switch (c)
        {
            // whitespace?
            case ' ' or '\r' or '\t' or '\n':
                break;

            // single character token?
            case char _ when _singleCharacterTokens.TryGetValue(c, out var tokenType):
                AddToken(tokenType);
                break;

            // 1-2 character token?
            case char _ when _oneOrTwoCharacterTokens.TryGetValue(c, out var x):
                AddToken(Match(x.Item2) ? x.Item3 : x.Item1);
                break;

            // slash or comment?
            case '/':
                ScanSlash();
                break;

            // string literal?
            case '"':
                ScanString();
                break;

            // number literal?
            case char _ when IsDigit(c):
                ScanNumber();
                break;

            // identifier?
            case char _ when IsAlpha(c):
                ScanIdentifier();
                break;

            default:
                Error($"Unexpected character: '{c}' ({(int)c})");
                break;
        }
    }

    private void Error(string message)
    {
        _errors.Add(new ScanError(_line, message));
    }

    private void AddToken(TokenType tokenType, object? literal = null)
    {
        _tokens.Add(new Token(tokenType, GetLexeme(), literal, _line));
    }

    private string GetLexeme()
    {
        return _source[_start.._current];
    }

    private bool IsAtEnd()
    {
        return _current >= _source.Length;
    }

    /// <summary>
    /// Consume current character.
    /// </summary>
    /// <returns></returns>
    private char Advance()
    {
        var c = _source[_current];

        _current++;

        if (c == '\n') _line++;

        return c;
    }

    /// <summary>
    /// Consume current character if it is equal to the passed character.
    /// </summary>
    /// <param name="c"></param>
    /// <returns></returns>
    private bool Match(char c)
    {
        if (IsAtEnd()) return false;

        if (Peek() == c)
        {
            _ = Advance();
            return true;
        }

        return false;
    }

    /// <summary>
    /// Peek at current character without consuming it.
    /// </summary>
    /// <returns></returns>
    private char Peek()
    {
        if (IsAtEnd()) return '\0';

        return _source[_current];
    }

    /// <summary>
    /// Peek at next character without consuming it.
    /// </summary>
    /// <returns></returns>
    private char PeekNext()
    {
        if (_current + 1 >= _source.Length) return '\0';

        return _source[_current + 1];
    }

    private void AdvanceTo(char terminator)
    {
        while (Peek() != terminator && !IsAtEnd())
        {
            Advance();
        }

        // Consume terminator.
        if (!IsAtEnd())
        {
            Advance();
        }
    }

    private void ScanSlash()
    {
        // Start of comment?
        if (Match('/'))
        {
            // Consume rest of the line.
            AdvanceTo('\n');
        }
        else
        {
            AddToken(TokenType.SLASH);
        }
    }

    private void ScanString()
    {
        // Consume everything to the closing quote.
        AdvanceTo('"');

        if (IsAtEnd())
        {
            Error("Unterminated string.");
            return;
        }

        // Get literal without quotes.
        var value = _source[(_start + 1) .. (_current - 1)];

        AddToken(TokenType.STRING, value);
    }

    private void ScanNumber()
    {
        // Consume digits before optional decimal point.
        while (IsDigit(Peek())) Advance();

        if (Peek() == '.' && IsDigit(PeekNext()))
        {
            // Consume optional decimal point.
            Advance();

            // Consume digits after optional decimal point.
            while (IsDigit(Peek())) Advance();
        }

        var lexeme = GetLexeme();

        if (Double.TryParse(lexeme, CultureInfo.InvariantCulture, out var value))
        {
            AddToken(TokenType.NUMBER, value);
        }
        else
        {
            Error("Invalid number.");
        }
    }

    private void ScanIdentifier()
    {
        // Note: first character is alpha, rest alphanumeric (see ScanToken).
        while (IsAlphaNumeric(Peek())) Advance();

        if (_keywords.TryGetValue(GetLexeme(), out var keyword))
        {
            AddToken(keyword);
        }
        else
        {
            AddToken(TokenType.IDENTIFIER);
        }
    }

    private static bool IsDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    private static bool IsAlpha(char c)
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
                c == '_';
    }

    private static bool IsAlphaNumeric(char c)
    {
        return IsDigit(c) || IsAlpha(c);
    }
}
