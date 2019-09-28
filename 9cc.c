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
  TK_EOF,         // End Of File
  NUM_TOKEN_KIND,
} TokenKind;

unsigned int TK_LEN_OF_KIND[NUM_TOKEN_KIND] = {
1,
1,
1,
1,
2,
2,
1,
2,
1,
2,
1,
};


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
  NUM_NODE_KIND,
} NodeKind;

const char* TOKEN_KIND_STR[NUM_TOKEN_KIND] =
  {"TK_OP", "TK_LPAR", "TK_RPAR", "TK_NUM", "TK_EQ", "TK_NE", "TK_LT", "TK_LE", "TK_GT", "TK_GE", "TK_EOF"};

const char* NODE_KIND_STR[NUM_NODE_KIND] =
  {"ND_ADD", "ND_SUB", "ND_MUL", "ND_DIV", "ND_NUM", "ND_LPAR", "ND_RPAR", "ND_EQ", "ND_NE", "ND_LT", "ND_LE", "ND_GT", "ND_GE"};

typedef struct Node Node;
// Node
struct Node {
  NodeKind kind; // kind of node
  Node *lhs;     // left leaf
  Node *rhs;     // right leaf
  int val;  // value when kind is ND_NUM
};


Node* expr();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
  debug_print("** new_node : %s\n", NODE_KIND_STR[kind]);
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->val  = 999999;
  node->lhs  = lhs;
  node->rhs  = rhs;
  return node;
}

Node* new_node_number(int val) {
  debug_print("** new_node_number : %d\n", val);
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
bool consume(const char *str) {
  if (strncmp(str, token->str, token->len))
    return false;
  // next token
  debug_print("op: %c  token->kind: %d token->str[0]: %c\n", op, token->kind, token->str[0]);
  token = token->next;
  return true;
}

// go to next token and return true if current token is TK_OP and matches with op
// otherwise, throw error and exit application
void expect(const char op) {
  if ((token->kind != TK_OP && token->kind != TK_LPAR && token->kind != TK_RPAR)
      || token->str[0] != op)
    error("%c expected, but got %c", op, token->str[0]);
  // next token
  token = token->next;
}

// go to next token and return value if current token is TK_NUM
// otherwise, throw error and exit application
int expect_number() {
  if (token->kind != TK_NUM)
    error("number expected, but got %s", TOKEN_KIND_STR[token->kind]);
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
  fprintf(stderr, "kind: %d value: %d str[0]: %c\n", tkn->kind, tkn->val, tkn->str[0]);
}

void print_token_recursive(Token *tkn) {
  while (tkn->kind != TK_EOF) {
    print_token(tkn);
    tkn = tkn->next;
  }
}

void print_node(Node *node) {
  fprintf(stderr, "kind: %s value: %d\n", NODE_KIND_STR[node->kind], node->val);
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
Token* new_token(Token *cur, TokenKind kind, char **str, int len) {
  Token *next = (Token*)calloc(1, sizeof(Token));
  cur->next = next;
  next->kind = kind;
  next->str  = *str;
  next->len = len;
  if (kind == TK_NUM)
    next->val  = strtol(*str, str, 10);
  else
    *str = *str + len;
  return next;
}

bool get_kind(char *p, TokenKind *kind) {
  if (isoperation(*p)) {
    *kind = TK_OP;
    return true;
  }
  if (*p == '(') {
    *kind = TK_LPAR;
    return true;
  }
  if (*p == ')') {
    *kind = TK_RPAR;
    return true;
  }
  if (isdigit(*p)) {
    *kind = TK_NUM;
    return true;
  }
  if (strncmp(p, "==", 2) == 0) {
    *kind = TK_EQ;
    return true;
  }
  if (strncmp(p, "!=", 2) == 0) {
    *kind = TK_NE;
    return true;
  }
  if (strncmp(p, "<=", 2) == 0) {
    *kind = TK_LE;
    return true;
  }
  if (strncmp(p, ">=", 2) == 0) {
    *kind = TK_GE;
    return true;
  }
  if (*p == '<') {
    *kind = TK_LT;
    return true;
  }
  if (*p == '>') {
    *kind = TK_GT;
    return true;
  }

  return false;
}

Token* tokenize(char *p) {
  int len;
  TokenKind kind;
  Token head;
  Token *cur = &head;

  while (*p) {
    // skip some characters;
    if (isskip(*p)) {
      p++;
      continue;
    }
    if (get_kind(p, &kind)) {
      len = TK_LEN_OF_KIND[kind];
      cur = new_token(cur, kind, &p, len);
      continue;
    }
    error("cannot tokenize");
  }

  cur = new_token(cur, TK_EOF, &p, 1);

  return head.next;
}

// return true if current token is eof
bool is_eof(Token* tkn) {
  return tkn->kind == TK_EOF;
}

// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-") primary
// primary    = num | "(" expr ")"

Node* expr() {
  debug_put("expr\n");
  return equality();
}

// equality   = relational ("==" relational | "!=" relational)*
Node* equality() {
  debug_put("equality\n");
  Node *node = relational();
  Node *rhs;

  while (1) {
    if (consume("==")) {
      rhs  = relational();
      node = new_node(ND_EQ, node, rhs);
    } else if (consume("!=")) {
      rhs = relational();
      node = new_node(ND_NE, node, rhs);
    } else {
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node* relational() {
  debug_put("relational\n");
  Node *node = add();
  Node *rhs;

  while (1) {
    if (consume("<")) {
      rhs  = add();
      node = new_node(ND_LT, node, rhs);
    } else if (consume("<=")) {
      rhs  = add();
      node = new_node(ND_LE, node, rhs);
    } else if (consume(">")) {
      rhs  = add();
      node = new_node(ND_GT, node, rhs);
    } else if (consume(">=")) {
      rhs  = add();
      node = new_node(ND_GE, node, rhs);
    } else {
      return node;
    }
  }

}

// add        = mul ("+" mul | "-" mul)*
Node* add() {
  debug_put("add\n");
  Node *node = mul();
  Node *rhs;

  while (1) {
    if (consume("+")) {
      rhs  = mul();
      node = new_node(ND_ADD, node, rhs);
    } else if (consume("-")) {
      rhs = mul();
      node = new_node(ND_SUB, node, rhs);
    } else {
      return node;
    }
  }
}

// mul     = unary ("*" unary | "/" unary)*
Node* mul() {
  debug_put("mul\n");
  Node *node = unary();
  Node *rhs;

  while (1) {
    if (consume("*")) {
      rhs = unary();
      node = new_node(ND_MUL, node, rhs);
    } else if (consume("/")) {
      rhs = unary();
      node = new_node(ND_DIV, node, rhs);
    } else {
      return node;
    }
  }
}

// unary   = ("+" | "-") primary
Node* unary() {
  debug_put("unary\n");
  
  if (consume("+"))
    return primary();
  if (consume("-"))
    return new_node(ND_SUB, new_node_number(0), primary());

  return primary();
}

// primary = num | "(" expr ")"
Node* primary() {
  debug_put("primary\n");
  // if '(' is found, 
  if (consume("(")) {
    Node *node = expr();
    expect(')');   // ')' should be here
    return node;
  } else {
    return new_node_number(expect_number());
  }
}

void gen(Node *node) {
  if (node == NULL)
    return ;
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
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
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_NE:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LT:
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_GT:
      printf("  cmp rax, rdi\n");
      printf("  setg al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_GE:
      printf("  cmp rax, rdi\n");
      printf("  setge al\n");
      printf("  movzx rax, al\n");
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
  // print_token_recursive(token);

  Node* node = expr();
  // print_node_recursive(node);

  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}