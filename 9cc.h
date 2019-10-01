#ifndef __9CC_H__
#define __9CC_H__

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#define debug_print(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#define debug_put(str) fprintf(stderr, str)
#else
#define debug_print(fmt, ...)
#define debug_put(str)
#endif

#define MAX_ARGS 6

extern const char* X86_64_ABI_REG[MAX_ARGS];

// kind of token
typedef enum {
  TK_ADD,         // addition
  TK_SUB,         // subtraction
  TK_MUL,         // multiplication
  TK_DIV,         // division
  TK_LPAR,        // left parenthesis "("
  TK_RPAR,        // right parenthesis ")"
  TK_LBRA,        // left braces  "{"
  TK_RBRA,        // right braces "}"
  TK_NUM,         // digits
  TK_EQ,          // "=="
  TK_NE,          // "!="
  TK_LT,          // "<
  TK_LE,          // "<=
  TK_GT,          // ">
  TK_GE,          // ">=
  TK_IDENT,       // variable
  TK_ASSIGN,      // assignment "="
  TK_SEMICOLON,   // semicolon ";"
  TK_COMMA,       // comma ","
  TK_RETURN,      // return
  TK_IF,          // if
  TK_ELSE,        // else
  TK_WHILE,       // while
  TK_FOR,         // for
  TK_EOF,         // End Of File
  NUM_TOKEN_KIND,
} TokenKind;

typedef struct Token Token;
// Token
struct Token {
  TokenKind kind; // kind of token
  Token *next;    // pointer to next token
  int val;        // value when kind is TK_NUM
  char *str;      // string of token
  int len;        // length of *str
};

// kind of Node
typedef enum {
  ND_ADD,         // addition
  ND_SUB,         // subtraction
  ND_MUL,         // multiplication
  ND_DIV,         // division
  ND_NUM,         // digits
  ND_LPAR,        // left parenthesis "("
  ND_RPAR,        // right parenthesis ")"
  ND_EQ,          // "=="
  ND_NE,          // "!="
  ND_LT,          // "<
  ND_LE,          // "<=
  ND_GT,          // ">
  ND_GE,          // ">=
  ND_IDENT,       // variable
  ND_ASSIGN,      // assignment "="
  ND_RETURN,      // return
  ND_IF,          // if
  ND_WHILE,       // while
  ND_FOR,         // for
  ND_BLOCK,       // "{" block "}"
  ND_FUNC_CALL,   // function call
  ND_FUNC_DEF,    // function definition
  NUM_NODE_KIND,
} NodeKind;

const char* TOKEN_KIND_STR[NUM_TOKEN_KIND];
const char* NODE_KIND_STR[NUM_NODE_KIND];

typedef struct Node Node;
// Node
struct Node {
  NodeKind kind;          // kind of node
  Node *lhs;              // left leaf
  Node *rhs;              // right leaf
  Node *condition;        // used in if, for, while
  Node *statement;        // used in for, while
  Node *if_statement;     // used in if
  Node *else_statement;   // used in if
  Node *init;             // used in for
  Node *last;             // used in for
  Node *block[256];       // used to represent block of code
  Node *args[MAX_ARGS+1]; // currently, support 6 arguments
  char label_s[256];      // label used in assembly
  char label_e[256];      // label used in assembly
  char *func_name;        // function name used in assembly
  int val;                // value when kind is ND_NUM
  int offset;             // stack offset
};

// left value
typedef struct LVar LVar;

struct LVar {
  LVar *next;
  char *name;
  int len;
  int offset;
};

// current token
extern Token *token;
extern Node  *prog[];
extern LVar  *locals;

// parser
Token* tokenize(char *p);
void program();

// codegen
void gen(Node *node);

// utiliry
void error(char *fmt, ...);
void print_token(Token *tkn);
void print_token_recursive(Token *tkn);
void print_node(Node *node);
void print_node_recursive(Node *node);

#endif