using System.Collections.Frozen;
using System.Globalization;

namespace Lox.Tool;

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
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    EOF
}

public record Token(TokenType Type, string Lexeme, object? Literal);

public class Scanner
{
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
            { "and", TokenType.AND },
            { "class", TokenType.CLASS },
            { "else", TokenType.ELSE },
            { "false", TokenType.FALSE },
            { "fun", TokenType.FUN },
            { "for", TokenType.FOR },
            { "if", TokenType.IF },
            { "nil", TokenType.NIL },
            { "or", TokenType.OR },
            { "print", TokenType.PRINT },
            { "return", TokenType.RETURN },
            { "super", TokenType.SUPER },
            { "this", TokenType.THIS },
            { "true", TokenType.TRUE },
            { "var", TokenType.VAR },
            { "while", TokenType.WHILE },
        }
        .ToFrozenDictionary();

    private readonly string _source;

    private readonly List<Token> _tokens = [];

    private int _start = 0;
    private int _current = 0;
    private int _line = 1;

    public Scanner(string source)
    {
        _source = source;
    }

    public IEnumerable<Token> ScanTokens()
    {
        _tokens.Clear();

        while (!IsAtEnd())
        {
            ScanToken();
        }

        _tokens.Add(new Token(TokenType.EOF, String.Empty, null));

        return _tokens;
    }

    private void ScanToken()
    {
        _start = _current;

        var c = Advance();
        //Console.WriteLine($"c: '{c}' {(int)c}");

        switch (c)
        {
            case char _ when _singleCharacterTokens.TryGetValue(c, out var tokenType):
                AddToken(tokenType);
                break;

            case char _ when _oneOrTwoCharacterTokens.TryGetValue(c, out var x):
                AddToken(Match(x.Item2) ? x.Item3 : x.Item1);
                break;

            case '/':
                // Start of comment?
                if (Match('/'))
                {
                    // Consume rest of the line.
                    while (Peek() != '\n' && !IsAtEnd()) // TODO similar to ScanString() > make nice method?
                    {
                        Advance();
                    }
                }
                else
                {
                    AddToken(TokenType.SLASH);
                }
                break;

            // whitespace?
            case ' ':
            case '\r':
            case '\t':
                break;

            case '\n':
                _line++; // TODO maybe move this to Advance?
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
                Console.WriteLine($"Unexpected character: {c} {(int)c} at {_current}");
                break;
        }   
    }

    private void AddToken(TokenType tokenType, object? literal = null)
    {
        _tokens.Add(new Token(tokenType, GetLexeme(), literal));
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

    private void ScanString()
    {
        // Consume everything to the closing quote.
        while (Peek() != '"' && !IsAtEnd())
        {
            if (Peek() == '\n')
                _line++; // TODO maybe move this to Advance?

            Advance();
        }

        if (IsAtEnd())
        {
            Console.WriteLine("Unterminated string.");
            return;
        }

        // Consume closing quote.
        _ = Advance();

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
            Console.WriteLine($"Invalid number: '{lexeme}'");
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
