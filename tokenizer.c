#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "tokenizer.h"

typedef struct {
    Token *items;
    int count;
    int capacity;
} TokenList;

typedef struct {
    char **items;
    int count;
    int capacity;
} IndentStack;

static void add_token(TokenList *list, TokenType type, const char *value, int len, int line);
static void push_indent(IndentStack *stack, const char *indent);
static void pop_indent(IndentStack *stack);
static int is_keyword(const char *str, int len);

Token* tokenize(const char* code, int* token_count) {
    TokenList tokens = { .items = malloc(sizeof(Token) * 64), .count = 0, .capacity = 64 };
    IndentStack indent_stack = { .items = malloc(sizeof(char*) * 16), .count = 0, .capacity = 16 };
    if (!tokens.items || !indent_stack.items) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        free(tokens.items);
        free(indent_stack.items);
        return NULL;
    }
    push_indent(&indent_stack, "");

    const char *cursor = code;
    int line = 1;
    bool last_token_was_newline = true;

    int open_comments = 0;
    for(const char* p = code; *p; p++) {
        if (strncmp(p, ";-", 2) == 0) open_comments++;
        if (strncmp(p, "-;", 2) == 0) open_comments--;
    }
    if (open_comments != 0) {
        fprintf(stderr, "SyntaxError: Unbalanced multi-line comments.\n");
        free(tokens.items);
        while (indent_stack.count > 0) pop_indent(&indent_stack);
        free(indent_stack.items);
        return NULL;
    }

    while (*cursor != '\0') {
        const char *start = cursor;

        if (*cursor == '\n') {
            if (!last_token_was_newline) {
                add_token(&tokens, T_NEWLINE, "\\n", 2, line);
            }
            last_token_was_newline = true;

            cursor++;
            line++;
            while(*cursor == '\n') {
                cursor++;
                line++;
            }
            
            const char* indent_start = cursor;
            while (*cursor == ' ' || *cursor == '\t') {
                cursor++;
            }

            if (*cursor == '\n' || *cursor == '\0' || strncmp(cursor, ";-", 2) == 0 || *cursor == ';') {
                continue;
            }

            int indent_len = cursor - indent_start;
            char *current_indent = strndup(indent_start, indent_len);
            char *last_indent = indent_stack.items[indent_stack.count - 1];
            
            if (strcmp(current_indent, last_indent) != 0) {
                if (strlen(current_indent) > strlen(last_indent) && strncmp(current_indent, last_indent, strlen(last_indent)) == 0) {
                    add_token(&tokens, T_INDENT, "", 0, line);
                    last_token_was_newline = false;
                    push_indent(&indent_stack, current_indent);
                }
                else {
                    while (indent_stack.count > 1 && strcmp(indent_stack.items[indent_stack.count - 1], current_indent) != 0) {
                        add_token(&tokens, T_DEDENT, "", 0, line);
                        last_token_was_newline = false;
                        pop_indent(&indent_stack);
                    }
                    if (strcmp(indent_stack.items[indent_stack.count - 1], current_indent) != 0) {
                       fprintf(stderr, "IndentationError at line %d: unindent does not match any outer indentation level\n", line);
                       free(current_indent);
                       free_tokens(tokens.items, tokens.count);
                       while(indent_stack.count > 0) pop_indent(&indent_stack);
                       free(indent_stack.items);
                       return NULL;
                    }
                }
            }
            free(current_indent);
            continue;
        }

        if (isspace(*cursor)) {
            cursor++;
            continue;
        }

        if (strncmp(cursor, ";-", 2) == 0) {
            cursor += 2;
            while (*cursor != '\0' && strncmp(cursor, "-;", 2) != 0) {
                if (*cursor == '\n') line++;
                cursor++;
            }
            if (*cursor != '\0') cursor += 2;
            continue;
        }
        if (*cursor == ';') {
            while (*cursor != '\0' && *cursor != '\n') {
                cursor++;
            }
            continue;
        }
        
        last_token_was_newline = false;

        if (strncmp(cursor, "++", 2) == 0 || strncmp(cursor, "--", 2) == 0) { add_token(&tokens, T_INC_DEC, cursor, 2, line); cursor += 2; continue; }
        if (strncmp(cursor, "==", 2) == 0 || strncmp(cursor, "!=", 2) == 0 || strncmp(cursor, "<=", 2) == 0 || strncmp(cursor, ">=", 2) == 0) { add_token(&tokens, T_COMP_OP, cursor, 2, line); cursor += 2; continue; }
        if (strncmp(cursor, "+=", 2) == 0 || strncmp(cursor, "-=", 2) == 0 || strncmp(cursor, "*=", 2) == 0 || strncmp(cursor, "/=", 2) == 0 || strncmp(cursor, "%=", 2) == 0) { add_token(&tokens, T_COMP_ASSIGN, cursor, 2, line); cursor += 2; continue; }
        if (strncmp(cursor, "|>", 2) == 0) { add_token(&tokens, T_PIPE, cursor, 2, line); cursor += 2; continue; }

        if (*cursor == '"' || *cursor == '\'') {
            char quote_char = *cursor;
            cursor++;
            const char* string_start = cursor;
            while (*cursor != '\0' && (*cursor != quote_char || *(cursor - 1) == '\\')) {
                cursor++;
            }
            add_token(&tokens, T_STRING, string_start, cursor - string_start, line);
            if (*cursor == quote_char) cursor++;
            continue;
        }

        if (isdigit(*cursor)) {
            const char* num_start = cursor;
            while (isdigit(*cursor)) cursor++;
            if (*cursor == '.') {
                cursor++;
                while (isdigit(*cursor)) cursor++;
            }
            add_token(&tokens, T_NUMBER, num_start, cursor - num_start, line);
            continue;
        }

        if (isalpha(*cursor) || *cursor == '_') {
            const char* ident_start = cursor;
            while (isalnum(*cursor) || *cursor == '_') {
                cursor++;
            }
            int len = cursor - ident_start;
            TokenType type = is_keyword(ident_start, len) ? T_KEYWORD : T_IDENTIFIER;
            
            if (len == 4 && strncmp(ident_start, "true", 4) == 0) type = T_BOOL;
            else if (len == 5 && strncmp(ident_start, "false", 5) == 0) type = T_BOOL;
            else if (len == 3 && strncmp(ident_start, "and", 3) == 0) type = T_LOGIC_OP;
            else if (len == 2 && strncmp(ident_start, "or", 2) == 0) type = T_LOGIC_OP;
            else if (len == 3 && strncmp(ident_start, "not", 3) == 0) type = T_LOGIC_OP;

            add_token(&tokens, type, ident_start, len, line);
            continue;
        }
        
        switch (*cursor) {
            case '=': add_token(&tokens, T_ASSIGN, cursor, 1, line); cursor++; continue;
            case ':': add_token(&tokens, T_COLON, cursor, 1, line); cursor++; continue;
            case '!': add_token(&tokens, T_LOGIC_OP, cursor, 1, line); cursor++; continue;
            case '+': case '-': case '*': case '/': case '%': add_token(&tokens, T_OP, cursor, 1, line); cursor++; continue;
            case '<': case '>': add_token(&tokens, T_COMP_OP, cursor, 1, line); cursor++; continue;
            case '[': add_token(&tokens, T_LBRACKET, cursor, 1, line); cursor++; continue;
            case ']': add_token(&tokens, T_RBRACKET, cursor, 1, line); cursor++; continue;
            case '{': add_token(&tokens, T_LBRACE, cursor, 1, line); cursor++; continue;
            case '}': add_token(&tokens, T_RBRACE, cursor, 1, line); cursor++; continue;
            case ',': add_token(&tokens, T_COMMA, cursor, 1, line); cursor++; continue;
            case '.': add_token(&tokens, T_DOT, cursor, 1, line); cursor++; continue;
            case '(': add_token(&tokens, T_LPAREN, cursor, 1, line); cursor++; continue;
            case ')': add_token(&tokens, T_RPAREN, cursor, 1, line); cursor++; continue;
        }

        if (cursor == start) {
            fprintf(stderr, "SyntaxError: Illegal character '%c' at line %d\n", *cursor, line);
            free_tokens(tokens.items, tokens.count);
            while(indent_stack.count > 0) pop_indent(&indent_stack);
            free(indent_stack.items);
            return NULL;
        }
    }
    
    if (!last_token_was_newline) {
        add_token(&tokens, T_NEWLINE, "\\n", 2, line);
    }
    
    while (indent_stack.count > 1) {
        add_token(&tokens, T_DEDENT, "", 0, line);
        pop_indent(&indent_stack);
    }
    add_token(&tokens, T_EOF, "", 0, line);

    while(indent_stack.count > 0) pop_indent(&indent_stack);
    free(indent_stack.items);

    *token_count = tokens.count;
    return tokens.items;
}

static void add_token(TokenList *list, TokenType type, const char *value, int len, int line) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, sizeof(Token) * list->capacity);
    }
    list->items[list->count].type = type;
    list->items[list->count].value = strndup(value, len);
    list->items[list->count].line = line;
    list->count++;
}

static void push_indent(IndentStack *stack, const char *indent) {
    if (stack->count >= stack->capacity) {
        stack->capacity *= 2;
        stack->items = realloc(stack->items, sizeof(char*) * stack->capacity);
    }
    stack->items[stack->count] = strdup(indent);
    stack->count++;
}

static void pop_indent(IndentStack *stack) {
    if (stack->count > 0) {
        stack->count--;
        free(stack->items[stack->count]);
        stack->items[stack->count] = NULL;
    }
}

void free_tokens(Token* tokens, int token_count) {
    if (tokens == NULL) return;
    for (int i = 0; i < token_count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case T_MLCOMMENT: return "MLCOMMENT";
        case T_COMMENT: return "COMMENT";
        case T_NUMBER: return "NUMBER";
        case T_STRING: return "STRING";
        case T_BOOL: return "BOOL";
        case T_INC_DEC: return "INC_DEC";
        case T_COMP_OP: return "COMP_OP";
        case T_COMP_ASSIGN: return "COMP_ASSIGN";
        case T_ASSIGN: return "ASSIGN";
        case T_COLON: return "COLON";
        case T_PIPE: return "PIPE";
        case T_LOGIC_OP: return "LOGIC_OP";
        case T_KEYWORD: return "KEYWORD";
        case T_OP: return "OP";
        case T_IDENTIFIER: return "IDENTIFIER";
        case T_NEWLINE: return "NEWLINE";
        case T_LBRACKET: return "LBRACKET";
        case T_RBRACKET: return "RBRACKET";
        case T_LBRACE: return "LBRACE";
        case T_RBRACE: return "RBRACE";
        case T_COMMA: return "COMMA";
        case T_DOT: return "DOT";
        case T_LPAREN: return "LPAREN";
        case T_RPAREN: return "RPAREN";
        case T_INDENT: return "INDENT";
        case T_DEDENT: return "DEDENT";
        case T_EOF: return "EOF";
        case T_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static int is_keyword(const char *str, int len) {
    const char *keywords[] = {
        "start", "let", "if", "else", "while", "loop", "command", "object", 
        "check", "equals", "write", "ask", "as", "wait", "null", 
        "num", "text", "bool", "list", "map", "return", "in", "break", "continue"
    };
    int num_keywords = sizeof(keywords) / sizeof(char *);
    for (int i = 0; i < num_keywords; i++) {
        if (strlen(keywords[i]) == len && strncmp(str, keywords[i], len) == 0) {
            return 1;
        }
    }
    return 0;
}
