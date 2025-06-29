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

static Parser g_parser;

static Statement* parse_statement(Parser* p);
static Expression* parse_expression(Parser* p);

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

typedef Expression* (*ParseFn)(Parser*);
typedef Expression* (*InfixParseFn)(Parser*, Expression*);

typedef struct {
  ParseFn prefix;
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

static ParseRule* get_rule(TokenType type) {
    if (type == T_OP) {
        if (strcmp(current_token(&g_parser).value, "*") == 0 || strcmp(current_token(&g_parser).value, "/") == 0) {
            rules[T_OP].precedence = PREC_FACTOR;
        } else {
            rules[T_OP].precedence = PREC_TERM;
        }
    } else if (type == T_LOGIC_OP) {
        if (strcmp(current_token(&g_parser).value, "or") == 0) {
            rules[T_LOGIC_OP].precedence = PREC_OR;
        } else {
            rules[T_LOGIC_OP].precedence = PREC_AND;
        }
    }
    return &rules[type];
}


static Expression* parse_precedence(Parser* p, Precedence precedence) {
    advance(p);
    ParseFn prefix_rule = get_rule(previous_token(p).type)->prefix;
    if (prefix_rule == NULL) {
        fprintf(stderr, "ParseError on line %d: Expected expression.\n", previous_token(p).line);
        return NULL;
    }

    Expression* expr = prefix_rule(p);

    while (precedence <= get_rule(current_token(p).type)->precedence) {
        advance(p);
        InfixParseFn infix_rule = get_rule(previous_token(p).type)->infix;
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
            return NULL;
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
    ParseRule* rule = get_rule(operator.type);
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

Statement* parse_wait_statement(Parser* p) {
    Expression* seconds = parse_expression(p);
    consume(p, T_NEWLINE, "Expect newline after wait statement.");

    Statement* stmt = malloc(sizeof(Statement));
    stmt->base.node_type = NODE_TYPE_STATEMENT;
    stmt->base.line = previous_token(p).line;
    stmt->type = STMT_WAIT;
    stmt->as.wait_stmt.seconds = seconds;
    return stmt;
}

Statement* parse_if_statement(Parser* p) {
    Expression* condition = parse_expression(p);
    consume(p, T_COLON, "Expect ':' after if condition.");
    consume(p, T_NEWLINE, "Expect newline after if.");
    consume(p, T_INDENT, "Expect indent after if.");

    Statement* stmt = malloc(sizeof(Statement));
    stmt->base.node_type = NODE_TYPE_STATEMENT;
    stmt->type = STMT_IF;
    stmt->as.if_stmt.condition = condition;
    
    stmt->as.if_stmt.body = malloc(sizeof(Statement*));
    stmt->as.if_stmt.body[0] = parse_statement(p);
    stmt->as.if_stmt.body_count = 1;

    consume(p, T_DEDENT, "Expect dedent after if block.");
    return stmt;
}


Statement* parse_statement(Parser* p) {
    if (match(p, 1, T_KEYWORD)) {
        Token keyword = previous_token(p);
        if (strcmp(keyword.value, "let") == 0) return parse_let_statement(p);
        if (strcmp(keyword.value, "if") == 0) return parse_if_statement(p);
        if (strcmp(keyword.value, "wait") == 0) return parse_wait_statement(p);
    }

    Statement* stmt = malloc(sizeof(Statement));
    stmt->base.node_type = NODE_TYPE_STATEMENT;
    stmt->type = STMT_EXPR;
    stmt->as.expr_stmt.expression = parse_expression(p);
    consume(p, T_NEWLINE, "Expect newline after expression.");
    return stmt;
}

ProgramNode* parse(Token* tokens, int token_count) {
    Parser parser = { .tokens = tokens, .count = token_count, .current = 0 };
    g_parser = parser;

    ProgramNode* program = malloc(sizeof(ProgramNode));
    program->base.node_type = NODE_TYPE_PROGRAM;
    program->base.line = 0;
    program->statements = NULL;
    program->count = 0;
    
    if(!is_at_end(&parser)) {
       program->statements = malloc(sizeof(Statement*));
       program->statements[0] = parse_statement(&parser);
       program->count = 1;
    }

    return program;
}

void free_ast(AstNode* node) {
    free(node);
}

void print_ast(AstNode* node) {
    printf("AST printing is not fully implemented yet.\n");
}
