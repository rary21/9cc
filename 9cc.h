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

// kind of token
typedef enum {
  TK_OP,          // operations
  TK_LPAR,        // left parenthesis "("
  TK_RPAR,        // right parenthesis ")"
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
  NUM_NODE_KIND,
} NodeKind;

const char* TOKEN_KIND_STR[NUM_TOKEN_KIND];
const char* NODE_KIND_STR[NUM_NODE_KIND];

typedef struct Node Node;
// Node
struct Node {
  NodeKind kind; // kind of node
  Node *lhs;     // left leaf
  Node *rhs;     // right leaf
  int val;       // value when kind is ND_NUM
  int offset;    // stack offset
};

// current token
extern Token* token;
extern Node* prog[];

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