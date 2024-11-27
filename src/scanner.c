// clox - scanner.c

#include "scanner.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

// Note(matt): Presumably, this struct is hidden in here so that it can't be
//  instantiated by scanner.h -- i.e. it is an implementation detail and not
//  part of the interface
typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

// Note(matt): Globals are bad in general!
Scanner scanner;

// Initialize the scanner
void initScanner(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

// Determine if a character is alphabetical
static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Determine if a character is a digit
static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

// Determine if the scanner is at the end of the file
static bool isAtEnd()
{
    return *scanner.current == '\0';
}

// Read and consume the current character, then advance the scanner to the next character
static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

// Read the current character without consuming it
static char peek()
{
    return *scanner.current;
}

// Read the next character without consuming it
static char peekNext()
{
    if (isAtEnd()) {
        return '\0';
    }

    return scanner.current[1];
}

// Match the next token as part of a two-character token
static bool match(char expected)
{
    if (isAtEnd()) {
        return 0;
    }
    if (*scanner.current != expected) {
        return 0;
    }

    scanner.current++;
    return 1;
}

// Make a token from the stream
static Token makeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

// Return an error token along with an error message
static Token errorToken(const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

// Skips all whitespace
static void skipWhitespace()
{
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        // On encountering a line break, increment line count
        case '\n':
            scanner.line++;
            advance();
            break;
        // Match // for comments
        case '/':
            if (peekNext() == '/') {
                // A comment goes until the end of the line
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

// Check alphanumeric sequence against Lox keywords
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length
        && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

// Lex a alphanumeric sequence as its proper type
static TokenType identifierType()
{
    // Lex the sequence as one of the keywords in the Lox language...
    switch (scanner.start[0]) {
    case 'a':
        return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c':
        return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
        return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'a':
                return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'o':
                return checkKeyword(2, 1, "r", TOKEN_FOR);
            case 'u':
                return checkKeyword(2, 1, "n", TOKEN_FUN);
            }
        }
        break;
    case 'i':
        return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n':
        return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o':
        return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p':
        return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
        return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'h':
                return checkKeyword(2, 2, "is", TOKEN_THIS);
            case 'r':
                return checkKeyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'v':
        return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
        return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    // Else lex the sequence as an identifier
    return TOKEN_IDENTIFIER;
}

// Lex identifiers
static Token identifier()
{
    while (isAlpha(peek()) || isDigit(peek())) {
        advance();
    }

    return makeToken(identifierType());
}

// Lex numbers into a single token
static Token number()
{
    while (isDigit(peek())) {
        advance();
    }

    // Look for a fractional part
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the '.'
        advance();

        while (isDigit(peek())) {
            advance();
        }
    }

    return makeToken(TOKEN_NUMBER);
}

// Lex a string as a string token, to be read as a runtime value later
static Token string()
{
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            scanner.line++;
        }
    }

    if (isAtEnd()) {
        return errorToken("Unterminated string.");
    }

    // Read the closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

// Lex a scanned token
Token scanToken()
{
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }

    char c = advance();

    if (isAlpha(c)) {
        return identifier();
    }
    if (isDigit(c)) {
        return number();
    }

    switch (c) {

    // Single-character tokens
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE);
    case ';':
        return makeToken(TOKEN_SEMICOLON);
    case ',':
        return makeToken(TOKEN_COMMA);
    case '.':
        return makeToken(TOKEN_DOT);
    case '-':
        return makeToken(TOKEN_MINUS);
    case '+':
        return makeToken(TOKEN_PLUS);
    case '/':
        return makeToken(TOKEN_SLASH);
    case '*':
        return makeToken(TOKEN_STAR);

    // Two-character tokens
    case '!':
        return makeToken(
            match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
        return makeToken(
            match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
        return makeToken(
            match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
        return makeToken(
            match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

    case '"':
        return string();
    }

    return errorToken("Unexpected character.");
}
