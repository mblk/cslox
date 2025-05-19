#ifndef _clox_scanner_h_
#define _clox_scanner_h_

#include <stdint.h>

typedef struct {
    const char *start;
    const char *current;
    uint32_t line;
} scanner_t;

typedef enum {
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    // Keywords.
    TOKEN_AND, TOKEN_BREAK, TOKEN_CLASS, TOKEN_CONTINUE, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,
    TOKEN_ERROR,
    TOKEN_EOF
} token_type_t;

typedef struct {
    token_type_t type;
    const char* start;
    uint32_t length;
    uint32_t line;
} token_t;

void scanner_init(scanner_t* scanner, const char* source);

token_t scan_token(scanner_t* scanner);

const char* token_type_to_string(token_type_t type);



#endif
