// clox - compiler.c

#include "compiler.h"
#include "common.h"
#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR, // or
    PREC_AND, // and
    PREC_EQUALITY, // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM, // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // ! -
    PREC_CALL, // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

// Get the current chunk while it's compiling
static Chunk* currentChunk()
{
    return compilingChunk;
}

// Print the location of an error
static void errorAt(Token* token, const char* message)
{
    if (parser.panic_mode) {
        return;
    }
    parser.panic_mode = 1;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // DO NOTHING
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.had_error = 1;
}

// Print an error at the previous location
static void error(const char* message)
{
    errorAt(&parser.previous, message);
}

// Print an error at the current location
static void errorAtCurrent(const char* message)
{
    errorAt(&parser.current, message);
}

// Grab the first token and parse it, then continue until the end
static void advance()
{
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(parser.current.start);
    }
}

// Read the next token and validate that it has an expected type
static void consume(TokenType type, const char* message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

// Emit a byte of bytecode
static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
    emitByte(byte1);
    emitByte(byte2);
}

// Emit a return bytecode
static void emitReturn()
{
    emitByte(OP_RETURN);
}

// Make a value into a constant
static uint8_t makeConstant(Value value)
{
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

// Parse and emit a constant
static void emitConstant(Value value)
{
    emitBytes(OP_CONSTANT, makeConstant(value));
}

// End compilation
static void endCompiler()
{
    emitReturn();
} // ENDCOMPILER

// Forward declarations
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

// Parse binary operators
static void binary()
{
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    default:
        return; // Unreachable
    }
}

// Parse parentheses as grouping markers
static void grouping()
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// Parse and emit a number
static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

// Parse unary operator
static void unary()
{
    TokenType operatorType = parser.previous.type;

    // Compile the operand
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction
    switch (operatorType) {
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    default:
        return; // Unreachable
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = { grouping, NULL, PREC_NONE },
    [TOKEN_RIGHT_PAREN] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_BRACE] = { NULL, NULL, PREC_NONE },
    [TOKEN_RIGHT_BRACE] = { NULL, NULL, PREC_NONE },
    [TOKEN_COMMA] = { NULL, NULL, PREC_NONE },
    [TOKEN_DOT] = { NULL, NULL, PREC_NONE },
    [TOKEN_MINUS] = { unary, binary, PREC_TERM },
    [TOKEN_PLUS] = { NULL, binary, PREC_TERM },
    [TOKEN_SEMICOLON] = { NULL, NULL, PREC_NONE },
    [TOKEN_SLASH] = { NULL, binary, PREC_FACTOR },
    [TOKEN_STAR] = { NULL, binary, PREC_FACTOR },
    [TOKEN_BANG] = { NULL, NULL, PREC_NONE },
    [TOKEN_BANG_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_GREATER] = { NULL, NULL, PREC_NONE },
    [TOKEN_GREATER_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_LESS] = { NULL, NULL, PREC_NONE },
    [TOKEN_LESS_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_IDENTIFIER] = { NULL, NULL, PREC_NONE },
    [TOKEN_STRING] = { NULL, NULL, PREC_NONE },
    [TOKEN_NUMBER] = { number, NULL, PREC_NONE },
    [TOKEN_AND] = { NULL, NULL, PREC_NONE },
    [TOKEN_CLASS] = { NULL, NULL, PREC_NONE },
    [TOKEN_ELSE] = { NULL, NULL, PREC_NONE },
    [TOKEN_FALSE] = { NULL, NULL, PREC_NONE },
    [TOKEN_FOR] = { NULL, NULL, PREC_NONE },
    [TOKEN_FUN] = { NULL, NULL, PREC_NONE },
    [TOKEN_IF] = { NULL, NULL, PREC_NONE },
    [TOKEN_NIL] = { NULL, NULL, PREC_NONE },
    [TOKEN_OR] = { NULL, NULL, PREC_NONE },
    [TOKEN_PRINT] = { NULL, NULL, PREC_NONE },
    [TOKEN_RETURN] = { NULL, NULL, PREC_NONE },
    [TOKEN_SUPER] = { NULL, NULL, PREC_NONE },
    [TOKEN_THIS] = { NULL, NULL, PREC_NONE },
    [TOKEN_TRUE] = { NULL, NULL, PREC_NONE },
    [TOKEN_VAR] = { NULL, NULL, PREC_NONE },
    [TOKEN_WHILE] = { NULL, NULL, PREC_NONE },
    [TOKEN_ERROR] = { NULL, NULL, PREC_NONE },
    [TOKEN_EOF] = { NULL, NULL, PREC_NONE },
};

// Parse the precedence of an operator
static void parsePrecedence(Precedence precedence)
{
    advance();
    ParseFn prefix_rule = getRule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }

    // Call the prefix rule function
    prefix_rule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infix_rule = getRule(parser.previous.type)->infix;
        infix_rule();
    }
}

// Recursively called to compile an operand
static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

//
static ParseRule* getRule(TokenType type)
{
    return &rules[type];
}

// Compiles Lox tokens to VM bytecode
bool compile(const char* source, Chunk* chunk)
{
    initScanner(source);
    compilingChunk = chunk;

    // Grab one token at a time using the scanner
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.had_error;
}
