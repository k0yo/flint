#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser.h"

typedef struct {
    Token* tokens;
    int count;
    int current;
} Parser;

static Statement* parse_statement(Parser* p);
static Expression* parse_expression(Parser* p);
static void print_statement(Statement* stmt, int indent);
static void print_expression(Expression* expr, int indent);

static void advance(Parser* p) {
    if (p->current < p->count) {
        p->current++;
    }
}

static Token current_token(Parser* p) {
    return p->tokens[p->current];
}

static Token previous_token(Parser* p) {
    return p->tokens[p->current - 1];
}

static bool is_at_end(Parser* p) {
    return current_token(p).type == T_EOF;
}

static bool check(Parser* p, TokenType type) {
    if (is_at_end(p)) return false;
    return current_token(p).type == type;
}

static bool match(Parser* p, int n, ...) {
    va_list types;
    va_start(types, n);
    for (int i = 0; i < n; i++) {
        if (check(p, va_arg(types, TokenType))) {
            advance(p);
            va_end(types);
            return true;
        }
    }
    va_end(types);
    return false;
}

static Token consume(Parser* p, TokenType type, const char* message) {
    if (check(p, type)) {
        Token t = current_token(p);
        advance(p);
        return t;
    }
    fprintf(stderr, "ParseError on line %d: %s. Expected %s, got %s.\n",
        current_token(p).line, message,
        token_type_to_string(type), token_type_to_string(current_token(p).type));
    exit(1);
}

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,
  PREC_OR,
  PREC_AND,
  PREC_EQUALITY,
  PREC_COMPARISON,
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY,
  PREC_CALL,
  PREC_PRIMARY
} Precedence;

typedef Expression* (*PrefixParseFn)(Parser* p);
typedef Expression* (*InfixParseFn)(Parser* p, Expression* left);

typedef struct {
  PrefixParseFn prefix;
  InfixParseFn infix;
  Precedence precedence;
} ParseRule;

static Expression* primary(Parser* p);
static Expression* grouping(Parser* p);
static Expression* unary(Parser* p);
static Expression* binary(Parser* p, Expression* left);
static Expression* call(Parser* p, Expression* left);
static Expression* get(Parser* p, Expression* left);
static Expression* parse_precedence(Parser* p, Precedence precedence);

static ParseRule* get_rule(Parser* p, TokenType type);

ParseRule rules[] = {
  [T_LPAREN]      = {grouping, call,   PREC_CALL},
  [T_RPAREN]      = {NULL,     NULL,   PREC_NONE},
  [T_LBRACE]      = {NULL,     NULL,   PREC_NONE}, 
  [T_RBRACE]      = {NULL,     NULL,   PREC_NONE},
  [T_LBRACKET]    = {NULL,     NULL,   PREC_NONE}, 
  [T_RBRACKET]    = {NULL,     NULL,   PREC_NONE},
  [T_COMMA]       = {NULL,     NULL,   PREC_NONE},
  [T_DOT]         = {NULL,     get,    PREC_CALL},
  [T_OP]          = {unary,    binary, PREC_TERM},
  [T_LOGIC_OP]    = {unary,    binary, PREC_AND},
  [T_COMP_OP]     = {NULL,     binary, PREC_EQUALITY},
  [T_ASSIGN]      = {NULL,     NULL,   PREC_ASSIGNMENT},
  [T_IDENTIFIER]  = {primary,  NULL,   PREC_NONE},
  [T_NUMBER]      = {primary,  NULL,   PREC_NONE},
  [T_STRING]      = {primary,  NULL,   PREC_NONE},
  [T_BOOL]        = {primary,  NULL,   PREC_NONE},
  [T_KEYWORD]     = {NULL,     binary, PREC_COMPARISON},
  [T_EOF]         = {NULL,     NULL,   PREC_NONE},
};


static Expression* parse_precedence(Parser* p, Precedence precedence) {
    advance(p);
    PrefixParseFn prefix_rule = get_rule(p, previous_token(p).type)->prefix;
    if (prefix_rule == NULL) {
        fprintf(stderr, "ParseError on line %d: Expected expression.\n", previous_token(p).line);
        return NULL;
    }

    Expression* expr = prefix_rule(p);

    while (precedence <= get_rule(p, current_token(p).type)->precedence) {
        advance(p);
        InfixParseFn infix_rule = get_rule(p, previous_token(p).type)->infix;
        expr = infix_rule(p, expr);
    }

    return expr;
}

Expression* parse_expression(Parser* p) {
    return parse_precedence(p, PREC_ASSIGNMENT);
}

Expression* primary(Parser* p) {
    Expression* expr = malloc(sizeof(Expression));
    expr->base.node_type = NODE_TYPE_EXPRESSION;
    expr->base.line = previous_token(p).line;

    switch(previous_token(p).type) {
        case T_BOOL:
        case T_NUMBER:
        case T_STRING:
            expr->type = EXPR_LITERAL;
            expr->as.literal.literal = previous_token(p);
            break;
        case T_IDENTIFIER:
            expr->type = EXPR_IDENTIFIER;
            expr->as.identifier.identifier = previous_token(p);
            break;
        default:
            fprintf(stderr, "ParseError on line %d: Expected primary expression.\n", previous_token(p).line);
            free(expr);
            exit(1);
    }
    return expr;
}

Expression* grouping(Parser* p) {
    Expression* expr = malloc(sizeof(Expression));
    expr->base.node_type = NODE_TYPE_EXPRESSION;
    expr->base.line = previous_token(p).line;
    expr->type = EXPR_GROUPING;
    expr->as.grouping.expression = parse_expression(p);
    consume(p, T_RPAREN, "Expected ')' after expression.");
    return expr;
}

Expression* unary(Parser* p) {
    Token operator = previous_token(p);
    Expression* right = parse_precedence(p, PREC_UNARY);

    Expression* expr = malloc(sizeof(Expression));
    expr->base.node_type = NODE_TYPE_EXPRESSION;
    expr->base.line = operator.line;
    expr->type = EXPR_UNARY;
    expr->as.unary.op = operator;
    expr->as.unary.right = right;
    return expr;
}

Expression* binary(Parser* p, Expression* left) {
    Token operator = previous_token(p);
    ParseRule* rule = get_rule(p, operator.type);
    Expression* right = parse_precedence(p, (Precedence)(rule->precedence + 1));
    
    Expression* expr = malloc(sizeof(Expression));
    expr->base.node_type = NODE_TYPE_EXPRESSION;
    expr->base.line = operator.line;

    if (operator.type == T_KEYWORD && strcmp(operator.value, "in") == 0) {
        expr->type = EXPR_IN;
        expr->as.in_expr.left = left;
        expr->as.in_expr.op = operator;
        expr->as.in_expr.right = right;
    } else {
        expr->type = EXPR_BINARY;
        expr->as.binary.left = left;
        expr->as.binary.op = operator;
        expr->as.binary.right = right;
    }
    return expr;
}

Expression* call(Parser* p, Expression* left) {
    Expression* expr = malloc(sizeof(Expression));
    expr->base.node_type = NODE_TYPE_EXPRESSION;
    expr->base.line = previous_token(p).line;
    expr->type = EXPR_CALL;
    expr->as.call.callee = left;
    expr->as.call.args = NULL;
    expr->as.call.count = 0;
    consume(p, T_RPAREN, "Expect ')' after arguments.");
    return expr;
}

Expression* get(Parser* p, Expression* left) {
    Token name = consume(p, T_IDENTIFIER, "Expect property name after '.'.");
    
    Expression* expr = malloc(sizeof(Expression));
    expr->base.node_type = NODE_TYPE_EXPRESSION;
    expr->base.line = name.line;
    expr->type = EXPR_GET;
    expr->as.get.object = left;
    expr->as.get.name = name;
    return expr;
}

Statement* parse_let_statement(Parser* p) {
    Token name = consume(p, T_IDENTIFIER, "Expect variable name.");
    consume(p, T_ASSIGN, "Expect '=' after variable name.");
    Expression* initializer = parse_expression(p);
    consume(p, T_NEWLINE, "Expect newline after variable declaration.");

    Statement* stmt = malloc(sizeof(Statement));
    stmt->base.node_type = NODE_TYPE_STATEMENT;
    stmt->base.line = name.line;
    stmt->type = STMT_LET_ASSIGN;
    stmt->as.let_assign.name = name;
    stmt->as.let_assign.initializer = initializer;
    return stmt;
}

Statement* parse_write_statement(Parser* p) {
    Statement* stmt = malloc(sizeof(Statement));
    stmt->base.node_type = NODE_TYPE_STATEMENT;
    stmt->base.line = previous_token(p).line;
    stmt->type = STMT_WRITE;
    stmt->as.write_stmt.expression = parse_expression(p);
    consume(p, T_NEWLINE, "Expect newline after write statement.");
    return stmt;
}

Statement* parse_ask_statement(Parser* p) {
    Statement* stmt = malloc(sizeof(Statement));
    stmt->base.node_type = NODE_TYPE_STATEMENT;
    stmt->base.line = previous_token(p).line;
    stmt->type = STMT_ASK;
    stmt->as.ask_stmt.prompt = parse_expression(p);
    Token as_keyword = consume(p, T_KEYWORD, "Expect 'as' after ask prompt.");
    if (strcmp(as_keyword.value, "as") != 0) {
        fprintf(stderr, "ParseError on line %d: Expected 'as' keyword.\n", as_keyword.line);
        exit(1);
    }
    stmt->as.ask_stmt.variable = consume(p, T_IDENTIFIER, "Expect variable name after 'as'.");
    consume(p, T_NEWLINE, "Expect newline after ask statement.");
    return stmt;
}

Statement* parse_statement(Parser* p) {
    if (match(p, 1, T_KEYWORD)) {
        Token keyword = previous_token(p);
        if (strcmp(keyword.value, "let") == 0) return parse_let_statement(p);
        if (strcmp(keyword.value, "write") == 0) return parse_write_statement(p);
        if (strcmp(keyword.value, "ask") == 0) return parse_ask_statement(p);
    }

    Expression* expr = parse_expression(p);

    if (match(p, 1, T_ASSIGN)) {
        if (expr->type != EXPR_IDENTIFIER && expr->type != EXPR_GET) {
            fprintf(stderr, "ParseError on line %d: Invalid assignment target.\n", previous_token(p).line);
            exit(1);
        }
        Statement* stmt = malloc(sizeof(Statement));
        stmt->base.node_type = NODE_TYPE_STATEMENT;
        stmt->type = STMT_REASSIGN;
        stmt->as.reassign.target = expr;
        stmt->as.reassign.value = parse_expression(p);
        consume(p, T_NEWLINE, "Expect newline after assignment.");
        return stmt;
    }

    Statement* stmt = malloc(sizeof(Statement));
    stmt->base.node_type = NODE_TYPE_STATEMENT;
    stmt->type = STMT_EXPR;
    stmt->as.expr_stmt.expression = expr;
    consume(p, T_NEWLINE, "Expect newline after expression.");
    return stmt;
}

ProgramNode* parse(Token* tokens, int token_count) {
    Parser parser = { .tokens = tokens, .count = token_count, .current = 0 };

    ProgramNode* program = malloc(sizeof(ProgramNode));
    program->base.node_type = NODE_TYPE_PROGRAM;
    program->base.line = 0;
    program->statements = malloc(sizeof(Statement*) * 32);
    program->count = 0;
    int capacity = 32;

    Token start_keyword = consume(&parser, T_KEYWORD, "Program must start with 'start' keyword.");
    if (strcmp(start_keyword.value, "start") != 0) {
        fprintf(stderr, "ParseError: Program must start with 'start' keyword, got '%s'.\n", start_keyword.value);
        exit(1);
    }
    consume(&parser, T_COLON, "Expect ':' after 'start' keyword.");
    consume(&parser, T_NEWLINE, "Expect newline after 'start:'.");
    consume(&parser, T_INDENT, "Expect indented block after 'start:'.");

    while (!check(&parser, T_DEDENT) && !is_at_end(&parser)) {
        if (program->count >= capacity) {
            capacity *= 2;
            program->statements = realloc(program->statements, sizeof(Statement*) * capacity);
        }
        program->statements[program->count++] = parse_statement(&parser);
    }

    consume(&parser, T_DEDENT, "Expect dedent to close 'start' block.");

    return program;
}

void free_expression(Expression* expr) {
    if (expr == NULL) return;
    switch (expr->type) {
        case EXPR_BINARY:
            free_expression(expr->as.binary.left);
            free_expression(expr->as.binary.right);
            break;
        case EXPR_UNARY:
            free_expression(expr->as.unary.right);
            break;
        case EXPR_GROUPING:
            free_expression(expr->as.grouping.expression);
            break;
        case EXPR_GET:
            free_expression(expr->as.get.object);
            break;
        case EXPR_CALL:
            free_expression(expr->as.call.callee);
            break;
        case EXPR_LITERAL:
        case EXPR_IDENTIFIER:
            break;
        default: break;
    }
    free(expr);
}

void free_statement(Statement* stmt) {
    if (stmt == NULL) return;
    switch(stmt->type) {
        case STMT_LET_ASSIGN:
            free_expression(stmt->as.let_assign.initializer);
            break;
        case STMT_REASSIGN:
            free_expression(stmt->as.reassign.target);
            free_expression(stmt->as.reassign.value);
            break;
        case STMT_WRITE:
            free_expression(stmt->as.write_stmt.expression);
            break;
        case STMT_ASK:
            free_expression(stmt->as.ask_stmt.prompt);
            break;
        case STMT_EXPR:
            free_expression(stmt->as.expr_stmt.expression);
            break;
        default: break;
    }
    free(stmt);
}

void free_ast(AstNode* node) {
    if (node == NULL) return;
    ProgramNode* prog = (ProgramNode*)node;
    for (int i = 0; i < prog->count; i++) {
        free_statement(prog->statements[i]);
    }
    free(prog->statements);
    free(prog);
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static void print_expression(Expression* expr, int indent) {
    print_indent(indent);
    if (expr == NULL) {
        printf("(null)\n");
        return;
    }
    switch(expr->type) {
        case EXPR_BINARY:
            printf("BinaryOp(%s):\n", expr->as.binary.op.value);
            print_expression(expr->as.binary.left, indent + 1);
            print_expression(expr->as.binary.right, indent + 1);
            break;
        case EXPR_LITERAL:
            printf("Literal(%s)\n", expr->as.literal.literal.value);
            break;
        case EXPR_IDENTIFIER:
            printf("Identifier(%s)\n", expr->as.identifier.identifier.value);
            break;
        case EXPR_GET:
            printf("Get(%s):\n", expr->as.get.name.value);
            print_expression(expr->as.get.object, indent + 1);
            break;
        default:
            printf("UnknownExpr\n");
            break;
    }
}

static void print_statement(Statement* stmt, int indent) {
    print_indent(indent);
     if (stmt == NULL) {
        printf("(null statement)\n");
        return;
    }
    switch(stmt->type) {
        case STMT_LET_ASSIGN:
            printf("LetAssign(%s):\n", stmt->as.let_assign.name.value);
            print_expression(stmt->as.let_assign.initializer, indent + 1);
            break;
        case STMT_REASSIGN:
            printf("Reassign:\n");
            print_expression(stmt->as.reassign.target, indent + 1);
            print_expression(stmt->as.reassign.value, indent + 1);
            break;
        case STMT_WRITE:
            printf("Write:\n");
            print_expression(stmt->as.write_stmt.expression, indent + 1);
            break;
        case STMT_ASK:
            printf("Ask (as %s):\n", stmt->as.ask_stmt.variable.value);
            print_expression(stmt->as.ask_stmt.prompt, indent + 1);
            break;
        case STMT_EXPR:
            printf("ExprStmt:\n");
            print_expression(stmt->as.expr_stmt.expression, indent + 1);
            break;
        default:
            printf("UnknownStmt\n");
            break;
    }
}

void print_ast(AstNode* node) {
    if (node == NULL) return;
    ProgramNode* prog = (ProgramNode*)node;
    printf("--- Abstract Syntax Tree ---\n");
    printf("Program:\n");
    for (int i = 0; i < prog->count; i++) {
        print_statement(prog->statements[i], 1);
    }
    printf("--------------------------\n");
}

static ParseRule* get_rule(Parser* p, TokenType type) {
    if (type == T_KEYWORD) {
        if (strcmp(current_token(p).value, "in") == 0) {
            return &rules[T_KEYWORD];
        }
        return &rules[T_EOF];
    }

    if (type == T_OP) {
        if (strcmp(current_token(p).value, "*") == 0 || strcmp(current_token(p).value, "/") == 0) {
            rules[T_OP].precedence = PREC_FACTOR;
        } else {
            rules[T_OP].precedence = PREC_TERM;
        }
    } else if (type == T_LOGIC_OP) {
        if (strcmp(current_token(p).value, "or") == 0) {
            rules[T_LOGIC_OP].precedence = PREC_OR;
        } else {
            rules[T_LOGIC_OP].precedence = PREC_AND;
        }
    }
    return &rules[type];
}
