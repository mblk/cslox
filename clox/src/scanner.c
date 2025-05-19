#include "scanner.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

void scanner_init(scanner_t* scanner, const char* source)
{
    assert(scanner);
    assert(source);

    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
}

inline static char peek(const scanner_t* scanner)
{
    return *scanner->current;
}

static bool is_at_end(const scanner_t* scanner)
{
    return peek(scanner) == '\0';
}

static char peek_next(const scanner_t* scanner)
{
    if (is_at_end(scanner)) return '\0';
    return *(scanner->current + 1);
}

static char advance(scanner_t* scanner)
{
    char c = peek(scanner);
    scanner->current++;

    if (c == '\n') {
        scanner->line++;
    }

    return c;
}

static bool match(scanner_t* scanner, char expected)
{
    if (is_at_end(scanner)) return false;
    if (peek(scanner) != expected) return false;

    advance(scanner);
    return true;
}

static void skip_whitespace(scanner_t* scanner)
{
    for(;;) {
        const char c = peek(scanner);
        switch (c) {
            case '\n':
                //scanner->line++;
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;

            case '/':
                // comment?
                if (peek_next(scanner) == '/') {
                    // consume remaining line
                    while (peek(scanner) != '\n' && !is_at_end(scanner)) {
                        advance(scanner);
                    }
                } else {
                    return;
                }
                break;

            default: return;
        }
    }
}

static token_t make_token(const scanner_t* scanner, token_type_t type)
{
    token_t token;
    token.type = type;
    token.line = scanner->line;
    token.start = scanner->start;
    token.length = (uint32_t)(scanner->current - scanner->start);
    return token;
}

static token_t error_token(const scanner_t* scanner, const char* message)
{
    token_t token;
    token.type = TOKEN_ERROR;
    token.line = scanner->line;
    token.start = message;
    token.length = (uint32_t)strlen(message);
    return token;
}

static token_t string(scanner_t* scanner)
{
    while (peek(scanner) != '"' && !is_at_end(scanner)) {
        advance(scanner);
    }

    if (is_at_end(scanner)) {
        return error_token(scanner, "Unterminated string.");
    }

    advance(scanner); // closing quote

    return make_token(scanner, TOKEN_STRING);
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return c >= 'a' && c <= 'z' ||
           c >= 'A' && c <= 'Z' ||
           c == '_';
}

static bool is_alpha_numeric(char c)
{
    return is_digit(c) || is_alpha(c);
}

static token_t number(scanner_t *scanner)
{
    // digits before decimal point
    while (is_digit(peek(scanner))) {
        advance(scanner);
    }

    // optional decimal point
    if (peek(scanner) == '.' && is_digit(peek_next(scanner))) {
        // decimal point
        advance(scanner);

        // fractional part
        while (is_digit(peek(scanner))) {
            advance(scanner);
        }
    }

    return make_token(scanner, TOKEN_NUMBER);
}

static token_type_t check_keyword(const scanner_t* scanner, size_t start, size_t length, const char *rest, token_type_t type)
{
    size_t scanned_length = (size_t)(scanner->current - scanner->start);

    if (scanned_length == start + length) {
        if (memcmp(scanner->start + start, rest, length) == 0) {
            return type;
        }
    }

    return TOKEN_IDENTIFIER;
}

static token_type_t identifier_type(const scanner_t *scanner)
{
    switch(scanner->start[0]) {
        case 'a': return check_keyword(scanner, 1, 2, "nd", TOKEN_AND);
        case 'b': return check_keyword(scanner, 1, 4, "reak", TOKEN_BREAK);
        case 'e': return check_keyword(scanner, 1, 3, "lse", TOKEN_ELSE);
        case 'i': return check_keyword(scanner, 1, 1, "f", TOKEN_IF);
        case 'n': return check_keyword(scanner, 1, 2, "il", TOKEN_NIL);
        case 'o': return check_keyword(scanner, 1, 1, "r", TOKEN_OR);
        case 'p': return check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
        case 'r': return check_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
        case 's': return check_keyword(scanner, 1, 4, "uper", TOKEN_SUPER);
        case 'v': return check_keyword(scanner, 1, 2, "ar", TOKEN_VAR);
        case 'w': return check_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);

        case 'c':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'l': return check_keyword(scanner, 2, 3, "ass", TOKEN_CLASS);
                    case 'o': return check_keyword(scanner, 2, 6, "ntinue", TOKEN_CONTINUE);
                }
            }

        case 'f':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'a': return check_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(scanner, 2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(scanner, 2, 1, "n", TOKEN_FUN);
                }
            }
            break;

        case 't':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'h': return check_keyword(scanner, 2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(scanner, 2, 2, "ue", TOKEN_TRUE);
                }
            }
            break; 
    }

    return TOKEN_IDENTIFIER;
}

static token_t identifier(scanner_t* scanner)
{
    while (is_alpha_numeric(peek(scanner))) {
        advance(scanner);
    }

    return make_token(scanner, identifier_type(scanner));
}

token_t scan_token(scanner_t* scanner)
{
    assert(scanner);

    skip_whitespace(scanner);

    scanner->start = scanner->current;

    if (is_at_end(scanner)) {
        return make_token(scanner, TOKEN_EOF);
    }

    const char c = advance(scanner);

    if (is_digit(c)) return number(scanner);
    if (is_alpha(c)) return identifier(scanner);

    switch (c) {
        case '"': return string(scanner);

        case '(': return make_token(scanner, TOKEN_LEFT_PAREN);
        case ')': return make_token(scanner, TOKEN_RIGHT_PAREN);
        case '{': return make_token(scanner, TOKEN_LEFT_BRACE);
        case '}': return make_token(scanner, TOKEN_RIGHT_BRACE);
        case ';': return make_token(scanner, TOKEN_SEMICOLON);
        case ',': return make_token(scanner, TOKEN_COMMA);
        case '.': return make_token(scanner, TOKEN_DOT);
        case '-': return make_token(scanner, TOKEN_MINUS);
        case '+': return make_token(scanner, TOKEN_PLUS);
        case '/': return make_token(scanner, TOKEN_SLASH);
        case '*': return make_token(scanner, TOKEN_STAR);

        case '!': return make_token(scanner, match(scanner, '=') ? TOKEN_BANG_EQUAL    : TOKEN_BANG);
        case '=': return make_token(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL   : TOKEN_EQUAL);
        case '<': return make_token(scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL    : TOKEN_LESS);
        case '>': return make_token(scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    }

    return error_token(scanner, "Unexpected character.");
}

const char* token_type_to_string(token_type_t type)
{
    switch (type) {
        case TOKEN_LEFT_PAREN: return "LEFT_PAREN";
        case TOKEN_RIGHT_PAREN: return "RIGHT_PAREN";
        case TOKEN_LEFT_BRACE: return "LEFT_BRACE";
        case TOKEN_RIGHT_BRACE: return "RIGHT_BRACE";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_STAR: return "STAR";
        case TOKEN_BANG: return "BANG";
        case TOKEN_BANG_EQUAL: return "BANG_EQUAL";
        case TOKEN_EQUAL: return "EQUAL";
        case TOKEN_EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TOKEN_GREATER: return "GREATER";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_LESS: return "LESS";
        case TOKEN_LESS_EQUAL: return "LESS_EQUAL";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_AND: return "AND";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CLASS: return "CLASS";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_FUN: return "FUN";
        case TOKEN_IF: return "IF";
        case TOKEN_NIL: return "NIL";
        case TOKEN_OR: return "OR";
        case TOKEN_PRINT: return "PRINT";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_SUPER: return "SUPER";
        case TOKEN_THIS: return "THIS";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_VAR: return "VAR";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_EOF: return "EOF";
        default:
            assert(!"Missing case in token_type_to_string");
            return "???";
    }
}