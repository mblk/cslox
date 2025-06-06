#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include "value.h"
#include "object.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define COMPILER_PRINT_CALLS


// Grammar:
//
//
// statement → exprStmt
//           | printStmt ;
//
// declaration → varDecl
//             | statement ;
//


typedef struct {
    scanner_t scanner;

    token_t current;
    token_t previous;

    bool had_error;
    bool panic_mode;

    object_root_t* root;

    chunk_t* current_chunk;

} parser_t; // TODO rename to compiler_t ?

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

typedef void (*parse_fn)(parser_t*, bool can_assign);

typedef struct {
    parse_fn prefix;
    parse_fn infix;
    precedence_t precedence;
} parse_rule_t;

static void grouping(parser_t*, bool);
static void literal(parser_t*, bool);
static void number(parser_t*, bool);
static void string(parser_t*, bool);
static void variable(parser_t*, bool);
static void unary(parser_t*, bool);
static void binary(parser_t*, bool);
static void ternary(parser_t*, bool);

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
    [TOKEN_IDENTIFIER]      = {variable, NULL,   PREC_NONE},
    [TOKEN_STRING]          = {string,   NULL,   PREC_NONE},
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

static void parser_init(parser_t* parser, object_root_t* root, chunk_t* chunk, const char* source) {
    memset(parser, 0, sizeof(parser_t));

    scanner_init(&parser->scanner, source);

    parser->had_error = false;
    parser->panic_mode = false;

    parser->root = root;
    parser->current_chunk = chunk;

    // prime the parser
    advance(parser);
}

//
// error handling
//

static void error_at_token(parser_t* parser, const token_t* token, const char *message) {
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

static void error_at_current(parser_t* parser, const char* message) {
    error_at_token(parser, &parser->current, message);
}

static void error_at_previous(parser_t* parser, const char *message) {
    error_at_token(parser, &parser->previous, message);
}

//
// parse utils
//

static void advance(parser_t* parser) {
    parser->previous = parser->current;

    for(;;) {
        parser->current = scan_token(&parser->scanner);
        if (parser->current.type != TOKEN_ERROR) break;

        error_at_current(parser, parser->current.start);
    }
}

static void consume(parser_t* parser, token_type_t type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    error_at_current(parser, message);
}

static bool check(parser_t* parser, token_type_t type) {
    return parser->current.type == type;
}

static bool match(parser_t* parser, token_type_t type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    } else {
        return false;
    }
}

//
// emit utils
//

static void emit_byte(parser_t* parser, uint8_t byte) {
    chunk_write8(parser->current_chunk, byte, parser->previous.line);
}

static void emit_bytes(parser_t* parser, uint8_t byte1, uint8_t byte2) {
    emit_byte(parser, byte1);
    emit_byte(parser, byte2);
}

static void emit_long(parser_t* parser, uint32_t value) {
    chunk_write32(parser->current_chunk, value, parser->previous.line);
}

static void emit_byte_and_long(parser_t* parser, uint8_t value1, uint32_t value2) {
    emit_byte(parser, value1);
    emit_long(parser, value2);
}

static void emit_return(parser_t* parser) {
    emit_byte(parser, OP_RETURN);
}

// add to value table and load onto stack.
static void emit_const(parser_t* parser, value_t value) {
    const uint32_t index = chunk_add_value(parser->current_chunk, value);

    if (index < 256) {
        emit_bytes(parser, OP_CONST, (uint8_t)index);
    } else {
        emit_byte_and_long(parser, OP_CONST_LONG, index);
    }
}

// add to value table and return index.
static uint32_t emit_string_value(parser_t* parser, const token_t* name) {
    const string_object_t* obj = create_string_object(parser->root, name->start, name->length);
    const value_t value = OBJECT_VALUE((object_t*)obj);
    const size_t index = chunk_add_value(parser->current_chunk, value);

    return index;
}

static void emit_define_global(parser_t* parser, uint32_t name_index) {    
    if (name_index < 256) {
        emit_bytes(parser, OP_DEFINE_GLOBAL, (uint8_t)name_index);
    } else {
        emit_byte_and_long(parser, OP_DEFINE_GLOBAL_LONG, name_index);
    }
}

static void emit_get_global(parser_t* parser, uint32_t name_index) {
    if (name_index < 256) {
        emit_bytes(parser, OP_GET_GLOBAL, (uint8_t)name_index);
    } else {
        emit_byte_and_long(parser, OP_GET_GLOBAL_LONG, name_index);
    }
}

static void emit_set_global(parser_t* parser, uint32_t name_index) {
    if (name_index < 256) {
        emit_bytes(parser, OP_SET_GLOBAL, (uint8_t)name_index);
    } else {
        emit_byte_and_long(parser, OP_SET_GLOBAL_LONG, name_index);
    }
}

//
// Pratt parser
//

static const parse_rule_t* get_rule(token_type_t type) {
    const size_t index = (size_t)type;
    const size_t num_rules = sizeof(g_rules) / sizeof(parse_rule_t);

    assert(index < num_rules);

    return &g_rules[index];
}

#ifdef COMPILER_PRINT_CALLS
static int g_call_id = 1;
static int g_indent = 0;
static void print_indent() {
    for (int i=0; i<g_indent; i++) printf("    ");
}
#define pp_printf(args...) print_indent(); printf(args)
#endif

static void parse_precendence(parser_t* parser, precedence_t prec) {
#ifdef COMPILER_PRINT_CALLS
    const int id = g_call_id++;
    pp_printf("parse_precendence[%d] start prec=%d\n", id, (int)prec);
    g_indent++;
    pp_printf("prev=%s curr=%s\n", token_type_to_string(parser->previous.type), token_type_to_string(parser->current.type));
#endif

    advance(parser);

    // special handling for assignment to prevent constructs like: a * b = c + d;
    const bool can_assign = prec <= PREC_ASSIGNMENT;

    const parse_fn prefix = get_rule(parser->previous.type)->prefix;
    if (prefix == NULL) {
        error_at_previous(parser, "Expect expression.");
        return;
    }
#ifdef COMPILER_PRINT_CALLS
    pp_printf("...calling prefix %s\n", token_type_to_string(parser->previous.type));
#endif
    prefix(parser, can_assign);

    // consume tokens as long as the prec of their rule is >= 'prec'
    // stop if the prec of the next thing is less than 'prec'
    while (get_rule(parser->current.type)->precedence >= prec) {
        advance(parser);

        parse_fn infix = get_rule(parser->previous.type)->infix;
#ifdef COMPILER_PRINT_CALLS
        pp_printf("...calling infix %s\n", token_type_to_string(parser->previous.type));
#endif
        assert(infix);
        infix(parser, can_assign);
    }

    if (can_assign && match(parser, TOKEN_EQUAL)) {
        error_at_previous(parser, "Invalid assignment target.");
    }

#ifdef COMPILER_PRINT_CALLS
    g_indent--;
    pp_printf("parse_precendence[%d] end\n", id);
#undef pp_printf
#endif
}

static size_t parse_variable_name(parser_t* parser, const char* error_message) {
    consume(parser, TOKEN_IDENTIFIER, error_message);
    return emit_string_value(parser, &parser->previous);
}

//
// Functions for pratt parser
//

static void expression(parser_t* parser);

static void literal(parser_t *parser, [[maybe_unused]] bool can_assign) {
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

static void number(parser_t* parser, [[maybe_unused]] bool can_assign) {
    const double value = strtod(parser->previous.start, NULL);
    emit_const(parser, NUMBER_VALUE(value));
}

static void string(parser_t* parser, [[maybe_unused]] bool can_assign) {
    const char* const chars = parser->previous.start + 1;
    const size_t length = parser->previous.length - 2;

    const string_object_t* obj = create_string_object(parser->root, chars, length);
    const value_t value = OBJECT_VALUE((object_t*)obj);

    emit_const(parser, value);
}

static void variable(parser_t* parser, bool can_assign) {
    // index of name on value table
    const uint32_t name_index = emit_string_value(parser, &parser->previous);

    // get variable or set variable
    // a.b.c
    // a.b.c = ...
    // depending on whether a '=' is found.

    if (can_assign && match(parser, TOKEN_EQUAL)) {
        expression(parser);
        emit_set_global(parser, name_index);
    } else {
        emit_get_global(parser, name_index);
    }
}

static void grouping(parser_t* parser, [[maybe_unused]] bool can_assign) {
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(parser_t* parser, [[maybe_unused]] bool can_assign) {
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

static void binary(parser_t* parser, [[maybe_unused]] bool can_assign) {
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

static void ternary(parser_t* parser, [[maybe_unused]] bool can_assign) {
    // A ? B : C
    // left operand and '?' already consumed

    // compile then branch
    parse_precendence(parser, PREC_TERNARY);

    consume(parser, TOKEN_COLON, "Expect ':' after then branch of ternary operator.");

    // compile else branch
    parse_precendence(parser, PREC_ASSIGNMENT);

    // TODO generate matching bytecode
}

//
// error recovery
//

static void synchronize(parser_t* parser) {

    for(;;) {

        if (parser->previous.type == TOKEN_SEMICOLON) {
            // found good sync point
            return;
        }

        switch (parser->current.type) {
            case TOKEN_EOF:
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                // found good sync point
                parser->panic_mode = false;
                return;

            default:
                // continue looking
                break;
        }

        // skip token
        advance(parser);
    }
}

//
// Recursive descent parser
//

static void declaration(parser_t *parser);
static void statement(parser_t* parser);
static void print_statement(parser_t* parser);
static void expression_statement(parser_t* parser);

static void expression(parser_t* parser) {
    parse_precendence(parser, PREC_ASSIGNMENT);
}

static void var_declaration(parser_t* parser) {
    // var a = b;
    // var a;
    // "var" already consumed

    const uint32_t global_id = parse_variable_name(parser, "Expect variable name.");

    if (match(parser, TOKEN_EQUAL)) {
        expression(parser);
    } else {
        emit_byte(parser, OP_NIL); // default value
    }

    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    emit_define_global(parser, global_id);
}

static void declaration(parser_t *parser) {
    if (match(parser, TOKEN_VAR)) {
        var_declaration(parser);
    } else {
        statement(parser);
    }

    if (parser->panic_mode) {
        synchronize(parser);
    }
}

static void statement(parser_t* parser) {
    if (match(parser, TOKEN_PRINT)) {
        print_statement(parser);
    } else {
        expression_statement(parser);
    }
}

static void print_statement(parser_t* parser) {
    // eg: print 1+2+3;

    // print token already consumed
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(parser, OP_PRINT);
}

static void expression_statement(parser_t* parser) {
    // eg: 1+2+3;

    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(parser, OP_POP); // discard value
}



//
// ...
//

bool compile(object_root_t* root, chunk_t* chunk, const char* source) {
    assert(root);
    assert(chunk);
    assert(source);

    //printf("compile: START %s END\n", source);

    parser_t parser;
    parser_init(&parser, root, chunk, source);

    // Expression parser:
    //expression(&parser);
    //emit_return(&parser);
    //consume(&parser, TOKEN_EOF, "Expect end of expression.");

    // Statement parser:
    while (!match(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    emit_return(&parser);

    return !parser.had_error;
}
