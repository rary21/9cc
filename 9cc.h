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
  TK_LPARE,       // left parenthesis "("
  TK_RPARE,       // right parenthesis ")"
  TK_LCBRA,       // left braces  "{"
  TK_RCBRA,       // right braces "}"
  TK_LBBRA,       // left braces  "["
  TK_RBBRA,       // right braces "]"
  TK_SQUOT,       // single quotation
  TK_DQUOT,       // double quotation
  TK_LITERAL,     // literal
  TK_AND,         // "&"
  TK_SHARP,       // "#"
  TK_BACKSLASH,   // "\\"
  TK_NEWLINE,     // "\n"
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
  TK_INT,         // int
  TK_CHAR,        // char
  TK_STRUCT,      // struct
  TK_DEFINE,      // define
  TK_SIZEOF,      // sizeof
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
  int literal_id; // literal id

  // for error report
  char *start;
  char *end;
  char *buf;
};

// kind of Node
typedef enum {
  ND_ADD,         // addition
  ND_SUB,         // subtraction
  ND_MUL,         // multiplication
  ND_DIV,         // division
  ND_NUM,         // digits
  ND_EQ,          // "=="
  ND_NE,          // "!="
  ND_LT,          // "<
  ND_LE,          // "<=
  ND_GT,          // ">
  ND_GE,          // ">=
  ND_IDENT,       // variable
  ND_LITERAL,     // literal
  ND_ASSIGN,      // assignment "="
  ND_RETURN,      // return
  ND_IF,          // if
  ND_WHILE,       // while
  ND_FOR,         // for
  ND_BLOCK,       // "{" block "}"
  ND_FUNC_CALL,   // function call
  ND_FUNC_DEF,    // function definition
  ND_ADDR,        // "&"
  ND_DEREF,       // "*"
  ND_CAST,        // cast
  ND_SIZEOF,      // sizeof
  ND_LVAR_DECL,   // local variable declaration
  ND_LVAR_INIT,   // local variable initialize
  ND_GVAR_DECL,   // global variable declaration
  ND_ARG_DECL,    // function argument declaration
  ND_POSTINC,     // postfix increment
  ND_NONE,        // do nothing
  NUM_NODE_KIND,
} NodeKind;

const char* TOKEN_KIND_STR[NUM_TOKEN_KIND];
const char* NODE_KIND_STR[NUM_NODE_KIND];

typedef struct Vector Vector;
struct Vector {
  void **elem;
  int len;
  int capacity;
  int index;
};

typedef struct Type Type;
struct Type {
  enum {INT, CHAR, PTR, ARRAY, STRUCT} ty;
  struct Type *ptr_to;
  int size;
  int array_size;
  Vector *members;
  bool is_global;
};

// local variable
typedef struct LVar LVar;
struct LVar {
  char *name;
  int len;
  int offset;
  Type *type;
};

typedef struct Node Node;
// Node
struct Node {
  NodeKind kind;               // kind of node
  Node *lhs;                   // left leaf
  Node *rhs;                   // right leaf
  Node *condition;             // used in if, for, while
  Node *statement;             // used in for, while
  Node *if_statement;          // used in if
  Node *else_statement;        // used in if
  Node *init;                  // used in for
  Node *last;                  // used in for
  Node *body;                  // body of function
  Node *expr;                  // used in postfix increment
  int locals_size;             // total size of local variables
  Node *block[256];            // used to represent block of code
  Node *args_call[MAX_ARGS+1]; // currently, support 6 arguments
  Node *args_def[MAX_ARGS+1];  // currently, support 6 arguments
  char label_s[256];           // label used in assembly
  char label_e[256];           // label used in assembly
  char *name;                  // name used in assembly
  char *func_name;             // function name used in assembly
  int val;                     // value when kind is ND_NUM
  int offset;                  // stack offset
  int literal_id;              // literal id
  LVar *var;                   // variable
  Type *type;                  // type
  Type *ret_type;              // return type for function
  Type *cast_to;               // cast to this type
  // for error report
  Token *token;                // token
};

// current token
extern Token *token;
extern Vector *vec_token;
extern Node  *prog[];
extern Type  *ptr_types[];

// parser
Token* tokenize(char *p);
Vector *preprocess();
void program();
void sema();

// codegen
void gen(Node *node);

// utiliry
void error(char *fmt, ...);
void print_token(Token *tkn);
void print_token_recursive(Token *tkn);
void print_node(Node *node);
void print_node_recursive(Node *node);

Token* new_token(Token *cur, TokenKind kind, char **str);
Node* new_node(NodeKind kind, Node* lhs, Node* rhs);
Node* new_node_number(int val);
Type* new_type(int ty, Type *ptr_to);
Type* new_type_int();
Type* new_type_char();
Vector *new_vector();
void *vector_pop_front(Vector *vec);
void vector_push_back(Vector *vec, void *p);
void *vector_get(Vector *vec, int i);
void *vector_get_front(Vector *vec);

bool same_type(Type *t1, Type *t2);
bool same_size(Node *n1, Node *n2);
bool is_32(Node *node);
bool is_8(Node *node);

int get_line_number(Token *tkn);

#endif