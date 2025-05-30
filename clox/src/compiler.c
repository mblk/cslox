#include "compiler.h"
#include "chunk.h"
#include "scanner.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//#define COMPILER_PRINT_CALLS

typedef struct {
    scanner_t scanner;

    token_t current;
    token_t previous;

    bool had_error;
    bool panic_mode;

    chunk_t* current_chunk;

} parser_t;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_TERNARY,       // ?:
    PREC_OR,            // or
    PREC_AND,           // and
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_PRIMARY
} precedence_t;

typedef void (*parse_fn)(parser_t*);

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    precedence_t precedence;
} parse_rule_t;

static void grouping(parser_t*);
static void literal(parser_t*);
static void number(parser_t*);
static void unary(parser_t*);
static void binary(parser_t*);
static void ternary(parser_t*);

static const parse_rule_t g_rules[] = {
    [TOKEN_LEFT_PAREN]      = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]      = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RIGHT_BRACE]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]           = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]            = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]       = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]           = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]            = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]            = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]      = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_EQUAL]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]     = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]         = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL]   = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]            = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]      = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]      = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STRING]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NUMBER]          = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]              = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]             = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]              = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]            = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]             = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]             = {NULL,     NULL,   PREC_NONE},

    // experimental
    [TOKEN_QUESTION]        = {NULL,     ternary, PREC_TERNARY},
    [TOKEN_COLON]           = {NULL,     NULL,    PREC_NONE},

    // TODO break, continue
};

//
// init
//

static void advance(parser_t* parser);

static void parser_init(parser_t* parser, const char* source)
{
    memset(parser, 0, sizeof(parser_t));

    scanner_init(&parser->scanner, source);

    parser->had_error = false;
    parser->panic_mode = false;

    // prime the parser
    advance(parser);
}

//
// error handling
//

static void error_at_token(parser_t* parser, const token_t* token, const char *message)
{
    // skip extra errors in panic mode
    if (parser->panic_mode) return;

    fprintf(stderr, "[%u] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);

    parser->had_error = true;
    parser->panic_mode = true;
}

static void error_at_current(parser_t* parser, const char* message)
{
    error_at_token(parser, &parser->current, message);
}

static void error_at_previous(parser_t* parser, const char *message)
{
    error_at_token(parser, &parser->previous, message);
}

//
// parse utils
//

static void advance(parser_t* parser)
{
    parser->previous = parser->current;

    for(;;) {
        parser->current = scan_token(&parser->scanner);
        if (parser->current.type != TOKEN_ERROR) break;

        error_at_current(parser, parser->current.start);
    }
}

static void consume(parser_t* parser, token_type_t type, const char* message)
{
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    error_at_current(parser, message);
}

//
// emit utils
//

static void emit_byte(parser_t* parser, uint8_t byte)
{
    chunk_write8(parser->current_chunk, byte, parser->previous.line);
}

static void emit_bytes(parser_t* parser, uint8_t byte1, uint8_t byte2)
{
    emit_byte(parser, byte1);
    emit_byte(parser, byte2);
}

static void emit_return(parser_t* parser)
{
    emit_byte(parser, OP_RETURN);
}

static void emit_const(parser_t* parser, value_t value)
{
    chunk_write_const(parser->current_chunk, value, parser->previous.line);
}

//
// ...
//

static const parse_rule_t* get_rule(token_type_t type)
{
    const size_t index = (size_t)type;
    const size_t num_rules = sizeof(g_rules) / sizeof(parse_rule_t);

    assert(index < num_rules);

    return &g_rules[index];
}

#ifdef COMPILER_PRINT_CALLS
static int g_call_id = 1;
static int g_indent = 0;
static void print_indent()
{
    for (int i=0; i<g_indent; i++) printf("    ");
}
#define pp_printf(args...) print_indent(); printf(args)
#endif

static void parse_precendence(parser_t* parser, precedence_t prec)
{
#ifdef COMPILER_PRINT_CALLS
    const int id = g_call_id++;
    pp_printf("parse_precendence[%d] start prec=%d\n", id, (int)prec);
    g_indent++;
    pp_printf("prev=%s curr=%s\n", token_type_to_string(parser->previous.type), token_type_to_string(parser->current.type));
#endif

    advance(parser);

    const parse_fn prefix = get_rule(parser->previous.type)->prefix;
    if (prefix == NULL) {
        error_at_previous(parser, "Expect expression.");
        return;
    }
#ifdef COMPILER_PRINT_CALLS
    pp_printf("...calling prefix %s\n", token_type_to_string(parser->previous.type));
#endif
    prefix(parser);

    // consume tokens as long as the prec of their rule is >= 'prec'
    // stop if the prec of the next thing is less than 'prec'
    while (get_rule(parser->current.type)->precedence >= prec) {
        advance(parser);

        parse_fn infix = get_rule(parser->previous.type)->infix;
#ifdef COMPILER_PRINT_CALLS
        pp_printf("...calling infix %s\n", token_type_to_string(parser->previous.type));
#endif
        assert(infix);
        infix(parser);
    }

#ifdef COMPILER_PRINT_CALLS
    g_indent--;
    pp_printf("parse_precendence[%d] end\n", id);
#undef pp_printf
#endif
}

//
// ...
//

static void expression(parser_t* parser)
{
    parse_precendence(parser, PREC_ASSIGNMENT);
}

static void literal(parser_t *parser)
{
    const token_type_t type = parser->previous.type;

    switch (type) {
        case TOKEN_NIL:
            emit_byte(parser, OP_NIL);
            break;
        case TOKEN_TRUE:
            emit_byte(parser, OP_TRUE);
            break;
        case TOKEN_FALSE:
            emit_byte(parser, OP_FALSE);
            break;
        default:
            assert(!"Missing case in literal()");
            break;
    }
}

static void number(parser_t* parser)
{
    const double value = strtod(parser->previous.start, NULL);
    emit_const(parser, NUMBER_VALUE(value));
}

static void grouping(parser_t* parser)
{
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(parser_t* parser)
{
    // op already consumed
    const token_type_t type = parser->previous.type;

    // operand
    parse_precendence(parser, PREC_UNARY);

    // operation
    switch (type) {
        case TOKEN_BANG: emit_byte(parser, OP_NOT); break;
        case TOKEN_MINUS: emit_byte(parser, OP_NEGATE); break;

        default: assert(!"unhandled token type in unary"); break;
    }
}

static void binary(parser_t* parser)
{
    // left operand and op already consumed
    const token_type_t type = parser->previous.type;

    // compile right operand
    const parse_rule_t* rule = get_rule(type);
    parse_precendence(parser, (precedence_t)(rule->precedence + 1));

    // operation
    switch (type) {

        case TOKEN_EQUAL_EQUAL:     emit_byte(parser, OP_EQUAL); break;
        case TOKEN_BANG_EQUAL:      emit_bytes(parser, OP_EQUAL, OP_NOT); break;
        case TOKEN_GREATER:         emit_byte(parser, OP_GREATER); break;
        case TOKEN_GREATER_EQUAL:   emit_bytes(parser, OP_LESS, OP_NOT); break;
        case TOKEN_LESS:            emit_byte(parser, OP_LESS); break;
        case TOKEN_LESS_EQUAL:      emit_bytes(parser, OP_GREATER, OP_NOT); break;

        case TOKEN_PLUS:            emit_byte(parser, OP_ADD); break;
        case TOKEN_MINUS:           emit_byte(parser, OP_SUB); break;
        case TOKEN_STAR:            emit_byte(parser, OP_MUL); break;
        case TOKEN_SLASH:           emit_byte(parser, OP_DIV); break;

        default: assert(!"Unhandled token type in binary"); break;
    }
}

static void ternary(parser_t* parser)
{
    // A ? B : C
    // left operand and '?' already consumed

    // compile then branch
    parse_precendence(parser, PREC_TERNARY);

    consume(parser, TOKEN_COLON, "Expect ':' after then branch of ternary operator.");

    // compile else branch
    parse_precendence(parser, PREC_ASSIGNMENT);
}

//
// ...
//

bool compile(chunk_t* chunk, const char* source)
{
    assert(chunk);
    assert(source);

    //printf("compile: START %s END\n", source);

    parser_t parser;
    parser_init(&parser, source);
    parser.current_chunk = chunk;

    expression(&parser);
    emit_return(&parser);

    consume(&parser, TOKEN_EOF, "Expect end of expression.");

    return !parser.had_error;
}
