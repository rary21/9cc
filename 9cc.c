#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#define debug_print(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
#define debug_print(fmt, ...)
#endif

// kind of token
typedef enum {
  TK_OP,          // operations
  TK_NUM,         // digits
  TK_EOF,         // End Of File
} TokenKind;
typedef struct Token Token;
// Token
struct Token {
  TokenKind kind; // kind of token
  Token *next;    // pointer to next token
  long int val;        // value when kind is TK_NUM
  char *str;      // string of token
};

// kind of Node
typedef enum {
  ND_ADD,         // addition
  ND_SUB,         // subtraction
  ND_MUL,         // multiplication
  ND_DIV,         // division
  ND_NUM,         // digits
} NodeKind;

const char* NODE_KIND_STR[] = {"ND_ADD", "ND_SUB", "ND_MUL", "ND_DIV", "ND_NUM"};

typedef struct Node Node;
// Node
struct Node {
  NodeKind kind; // kind of node
  Node *lhs;     // left leaf
  Node *rhs;     // right leaf
  long int val;  // value when kind is ND_NUM
};


Node* expr();
Node* mul();
Node* primary();

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->val  = 999999;
  node->lhs  = lhs;
  node->rhs  = rhs;
  return node;
}

Node* new_node_number(long int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val  = val;
  node->rhs  = NULL;
  node->lhs  = NULL;
  return node;
}

// current token
Token *token;

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// go to next token and return true if current token is TK_OP and matches with op
// otherwise, return false
bool consume(const char op) {
  if (token->kind != TK_OP || token->str[0] != op)
    return false;
  // next token
  debug_print("op: %c  token->kind: %d token->str[0]: %c\n", op, token->kind, token->str[0]);
  token = token->next;
  return true;
}

// go to next token and return true if current token is TK_OP and matches with op
// otherwise, throw error and exit application
void expect(const char op) {
  if (token->kind != TK_OP || token->str[0] != op)
    error("expect error");
  // next token
  token = token->next;
}

// go to next token and return value if current token is TK_NUM
// otherwise, throw error and exit application
int expect_number() {
  if (token->kind != TK_NUM)
    error("expect error");
  // next token
  int value = token->val;
  token = token->next;
  return value;
}

// return true if c is expected to skip
bool isskip(const char c) {
  if (c == ' ')
    return true;
  return false;
}

// return true if c is operation
bool isoperation(const char c) {
  if (c == '+')
    return true;
  if (c == '-')
    return true;
  if (c == '*')
    return true;
  if (c == '/')
    return true;
  return false;
}

void print_token(Token *tkn) {
  fprintf(stderr, "kind: %d value: %ld str[0]: %c\n", tkn->kind, tkn->val, tkn->str[0]);
}

void print_token_recursive(Token *tkn) {
  while (tkn->kind != TK_EOF) {
    print_token(tkn);
    tkn = tkn->next;
  }
}

void print_node(Node *node) {
  fprintf(stderr, "kind: %s value: %ld\n", NODE_KIND_STR[node->kind], node->val);
}

void print_node_recursive(Node *node) {
  if (node->lhs != NULL) {
    print_node_recursive(node->lhs);
  }
  print_node(node);
  if (node->rhs != NULL) {
    print_node_recursive(node->rhs);
  }
}

// append new token to cur and return pointer to new token
Token* new_token(Token *cur, TokenKind kind, char *str) {
  Token *next = (Token*)calloc(1, sizeof(Token));
  cur->next = next;
  next->kind = kind;
  next->str  = str;
  return next;
}

Token* tokenize(char *p) {
  Token head;
  Token *cur = &head;

  while (*p) {
    // skip some characters;
    if (isskip(*p)) {
      p++;
      continue;
    }
    if (isoperation(*p)) {
      cur = new_token(cur, TK_OP, p);
      p++;
      continue;
    }
    if (isdigit(*p)) {
      cur = new_token(cur, TK_NUM, p);
      cur->val  = strtol(p, &p, 10);
      continue;
    }
    error("cannot tokenize");
  }

  cur = new_token(cur, TK_EOF, p);

  return head.next;
}

// return true if current token is eof
bool is_eof(Token* tkn) {
  return tkn->kind == TK_EOF;
}

// expr    = mul ("+" mul | "-" mul)*
// mul     = primary ("*" primary | "/" primary)*
// primary = num | "(" expr ")"
Node* expr() {
  Node *lhs = mul();
  Node *node = lhs, *rhs;

  while (1) {
    if (consume('+')) {
      rhs  = mul();
      node = new_node(ND_ADD, node, rhs);
    } else if (consume('-')) {
      rhs = mul();
      node = new_node(ND_SUB, node, rhs);
    } else {
      return node;
    }
  }
  return lhs;
}

// mul     = primary ("*" primary | "/" primary)*
Node* mul() {
  Node *lhs = primary();
  Node *node = lhs, *rhs;

  while (1) {
    if (consume('*')) {
      rhs = primary();
      node = new_node(ND_MUL, node, rhs);
    } else if (consume('/')) {
      rhs = primary();
      node = new_node(ND_DIV, node, rhs);
    } else {
      return node;
    }
  }

  return lhs;
}

// primary = num | "(" expr ")"
Node* primary() {
  // TODO: support "(" and ")"
  return new_node_number(expect_number());
}

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %ld\n", node->val);
    return;
  }
  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");
  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    default:
      error("error in gen");
  }
  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  char* p = argv[1];
  token = tokenize(p);
  print_token_recursive(token);

  Node* node = expr();
  print_node_recursive(node);

  gen(node);
  // long int val = expect_number();
  // printf("  mov rax, %ld\n", val);
  // while (token->kind != TK_EOF) {
  //   if (consume('+')) {
  //     val = expect_number();
  //     printf("  add rax, %ld\n", val);
  //     continue;
  //   }
  //   if (consume('-')) {
  //     val = expect_number();
  //     printf("  sub rax, %ld\n", val);
  //     continue;
  //   }
  //   if (is_eof(token))
  //     break;
  //   error("error in main loop");
  // }

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}