#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include <stdbool.h>

typedef enum {
    NODE_TYPE_PROGRAM,
    NODE_TYPE_STATEMENT,
    NODE_TYPE_EXPRESSION
} AstNodeType;

typedef enum {
    STMT_LET_ASSIGN,
    STMT_REASSIGN,
    STMT_IF,
    STMT_WHILE,
    STMT_LOOP,
    STMT_COMMAND_DEF,
    STMT_CHECK,
    STMT_WRITE,
    STMT_ASK,
    STMT_WAIT,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_EXPR,
} StatementType;

typedef enum {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_IDENTIFIER,
    EXPR_LIST,
    EXPR_MAP,
    EXPR_CALL,
    EXPR_GET,
    EXPR_GROUPING,
    EXPR_IN,
} ExpressionType;

typedef struct AstNode {
    AstNodeType node_type;
    int line;
} AstNode;

struct Statement;
struct Expression;

typedef struct {
    AstNode base;
    struct Statement** statements;
    int count;
} ProgramNode;

typedef struct Expression {
    AstNode base;
    ExpressionType type;
    union {
        struct { struct Expression* left; Token op; struct Expression* right; } binary;
        struct { Token op; struct Expression* right; } unary;
        struct { Token literal; } literal;
        struct { Token identifier; } identifier;
        struct { struct Expression** elements; int count; } list;
        struct { struct Expression** keys; struct Expression** values; int count; } map;
        struct { struct Expression* callee; struct Expression** args; int count; } call;
        struct { struct Expression* object; Token name; } get;
        struct { struct Expression* expression; } grouping;
        struct { struct Expression* left; Token op; struct Expression* right; } in_expr;
    } as;
} Expression;

typedef struct Statement {
    AstNode base;
    StatementType type;
    union {
        struct { Token name; Expression* initializer; } let_assign;
        struct { Expression* target; Expression* value; } reassign;
        struct { Expression* condition; struct Statement** body; int body_count; struct Statement* else_branch; } if_stmt;
        struct { Expression* condition; struct Statement** body; int body_count; } while_stmt;
        struct { Expression* count; struct Statement** body; int body_count; } loop_stmt;
        struct { Token name; Token* params; int param_count; struct Statement** body; int body_count; } command_def;
        struct { Expression* condition; struct Statement** cases; int case_count; } check_stmt;
        struct { Expression* expression; } write_stmt;
        struct { Token prompt; Token variable; } ask_stmt;
        struct { Expression* seconds; } wait_stmt;
        struct { Expression* value; } return_stmt;
        struct { Expression* expression; } expr_stmt;
    } as;
} Statement;


ProgramNode* parse(Token* tokens, int token_count);
void free_ast(AstNode* node);
void print_ast(AstNode* node);

#endif
