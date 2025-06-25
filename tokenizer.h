#ifndef TOKENIZER_H
#define TOKENIZER_H

typedef enum {
    T_MLCOMMENT, T_COMMENT, T_NUMBER, T_STRING, T_BOOL,
    T_INC_DEC, T_COMP_OP, T_COMP_ASSIGN, T_ASSIGN, T_COLON,
    T_PIPE, T_LOGIC_OP, T_KEYWORD, T_OP, T_IDENTIFIER,
    T_NEWLINE, T_LBRACKET, T_RBRACKET, T_LBRACE, T_RBRACE,
    T_COMMA, T_DOT, T_LPAREN, T_RPAREN,
    T_INDENT, T_DEDENT, T_EOF, T_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
} Token;

const char* token_type_to_string(TokenType type);
Token* tokenize(const char* code, int* token_count);
void free_tokens(Token* tokens, int token_count);

#endif // TOKENIZER_H
