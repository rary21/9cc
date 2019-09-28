#include "9cc.h"

unsigned int TK_LEN_OF_KIND[NUM_TOKEN_KIND] = {
1,        // operations
1,        // left parenthesis "("
1,        // right parenthesis ")"
1,        // digits (must be 1 since this will be used in strncmp)
2,        // "=="
2,        // "!="
1,        // "<
2,        // "<=
1,        // ">
2,        // ">=
1,        // variable
1,        // assignment "="
1,        // semicolon ";"
1,        // End Of File
};

const char* TOKEN_KIND_STR[NUM_TOKEN_KIND] =
  {"TK_OP", "TK_LPAR", "TK_RPAR", "TK_NUM", "TK_EQ", "TK_NE", "TK_LT", "TK_LE", "TK_GT", "TK_GE", "TK_IDENT", "TK_ASSIGN", "TK_SEMICOLON", "TK_EOF"};

const char* NODE_KIND_STR[NUM_NODE_KIND] =
  {"ND_ADD", "ND_SUB", "ND_MUL", "ND_DIV", "ND_NUM", "ND_LPAR", "ND_RPAR", "ND_EQ", "ND_NE", "ND_LT", "ND_LE", "ND_GT", "ND_GE", "ND_IDENT", "ND_ASSIGN"};

Node* prog[100];

void program();
Node* statement();
Node* expr();
Node* assignment();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();

// go to next token and return true if current token is TK_OP and matches with op
// otherwise, return false
bool consume(const char *str) {
  if (strlen(str) != token->len)
    return false;
  if (strncmp(str, token->str, token->len) != 0)
    return false;
  // next token
  debug_print("** consuming op: %c  token->kind: %d token->str[0]: %c length: %d\n",
               str[0], token->kind, token->str[0], token->len);
  token = token->next;
  return true;
}

// go to next token and return true if current token is TK_IDENT
// otherwise, return false
bool consume_ident(char *id) {
  if (token->kind == TK_IDENT) {
    *id = token->str[0];
    token = token->next;
    return true;
  }
  return false;
}

// go to next token and return true if current token is TK_OP and matches with op
// otherwise, throw error and exit application
void expect(const char *str) {
  if (strncmp(str, token->str, token->len) != 0)
    error("%c expected, but got %c", str[0], token->str[0]);
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
  if (isspace(c))
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

// append new token to cur and return pointer to new token
Token* new_token(Token *cur, TokenKind kind, char **str) {
  Token *next = (Token*)calloc(1, sizeof(Token));
  int len = TK_LEN_OF_KIND[kind];
  cur->next  = next;
  next->kind = kind;
  next->str  = *str;
  next->len  = len;
  if (kind == TK_NUM)
    next->val  = strtol(*str, str, 10);
  else
    *str = *str + len;
  return next;
}

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

bool get_kind(char *p, TokenKind *kind) {
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
  if (*p == '<') {
    *kind = TK_LT;
    return true;
  }
  if (*p == '>') {
    *kind = TK_GT;
    return true;
  }
  if (*p == '=') {
    *kind = TK_ASSIGN;
    return true;
  }
  if ('a' <= *p && *p <= 'z') {
    *kind = TK_IDENT;
    return true;
  }
  if (*p == ';') {
    *kind = TK_SEMICOLON;
    return true;
  }
  return false;
}

Token* tokenize(char *p) {
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
      cur = new_token(cur, kind, &p);
      continue;
    }
    error("cannot tokenize");
  }

  cur = new_token(cur, TK_EOF, &p);

  return head.next;
}

// return true if current token is eof
bool is_eof() {
  return token->kind == TK_EOF;
}

// program    = statement*
// statement  = expr ";"
// expr       = assignment
// assignment = equality ("=" assignment)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | ident | "(" expr ")"

void program() {
  int i = 0;
  Node* node;
  for (i = 0; !is_eof() && i < (sizeof(prog) - 1); i++) {
    node = statement();
    prog[i] = node;
  }
  prog[i] = NULL;
}

// statement  = expr ";"
Node* statement() {
  debug_put("statement\n");
  Node* node = expr();
  expect(";");
  return node;
}

// expr       = assignment
Node* expr() {
  debug_put("expr\n");
  return assignment();
}

// assignment = equality ("=" assignment)?
Node* assignment() {
  debug_put("assignment\n");
  Node *node = equality();
  Node *rhs;

  if (consume("=")) {
    rhs  = assignment();
    node = new_node(ND_ASSIGN, node, rhs);
  }
  return node;
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

// primary = num | ident | "(" expr ")"
Node* primary() {
  debug_put("primary\n");
  char id;
  // if '(' is found, 
  if (consume("(")) {
    Node *node = expr();
    expect(")");   // ')' should be here
    return node;
  } else if (consume_ident(&id)) {
    Node* node = new_node(ND_IDENT, NULL, NULL);
    node->offset = (id - 'a' + 1) * 8;
    return node;
  } else {
    return new_node_number(expect_number());
  }
}