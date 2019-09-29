#include "9cc.h"

unsigned int TK_LEN_OF_KIND[NUM_TOKEN_KIND] = {
1,        // addition
1,        // subtraction
1,        // multiplication
1,        // division
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
6,        // return
1,        // End Of File
};

const char* TOKEN_KIND_STR[NUM_TOKEN_KIND] =
  {"TK_ADD", "TK_SUB", "TK_MUL", "TK_DIV", "TK_LPAR", "TK_RPAR", "TK_NUM", "TK_EQ",
   "TK_NE", "TK_LT", "TK_LE", "TK_GT", "TK_GE", "TK_IDENT", "TK_ASSIGN",
   "TK_SEMICOLON", "TK_RETURN", "TK_EOF"};

const char* NODE_KIND_STR[NUM_NODE_KIND] =
  {"ND_ADD", "ND_SUB", "ND_MUL", "ND_DIV", "ND_NUM", "ND_LPAR", "ND_RPAR",
   "ND_EQ", "ND_NE", "ND_LT", "ND_LE", "ND_GT", "ND_GE", "ND_IDENT", "ND_ASSIGN", "ND_RETURN"};

Node *prog[100];
LVar *locals;

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

Node* new_node(NodeKind, Node*, Node*);

// go to next token and return true if current token has same kind
// otherwise, return false
bool consume(TokenKind kind) {
  if (token->kind != kind)
    return false;
  // next token
  debug_print("** consuming token->kind: %s\n",
               TOKEN_KIND_STR[token->kind]);
  token = token->next;
  return true;
}

// return LVar* which has same name, or NULL
LVar* find_lvar(const char* str, int len) {
  LVar* lvar;
  for (lvar = locals; lvar; lvar = lvar->next) {
    // skip if name length is diffrent
    if (lvar->len != len)
      continue;
    if (strncmp(lvar->name, str, len) == 0)
      return lvar;
  }

  return NULL;
}

// go to next token and return true if current token is TK_IDENT
// otherwise, return false
bool consume_ident(Node** node) {
  if (token->kind == TK_IDENT) {
    *node = new_node(ND_IDENT, NULL, NULL);
    LVar* lvar = find_lvar(token->str, token->len);
    if (lvar) {
      (*node)->offset = lvar->offset;
    } else {
      LVar* new_lvar   = (LVar*)calloc(1, sizeof(LVar));
      new_lvar->name   = token->str;
      new_lvar->len    = token->len;
      new_lvar->next   = locals;
      if (locals)
        new_lvar->offset = locals->offset + 8;
      else
        new_lvar->offset = 8;
      locals           = new_lvar;
      (*node)->offset = new_lvar->offset;
    }
    token = token->next;
    return true;
  }
  return false;
}

// go to next token and return true if current token has same kind
// otherwise, throw error and exit application
void expect(TokenKind kind) {
  if (token->kind != kind)
    error("%s expected, but got %s", TOKEN_KIND_STR[kind], TOKEN_KIND_STR[token->kind]);
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

int get_ident_len(char* str) {
  int i = 0;
  // ident must starts with alphabet
  if (!isalpha(str[0]))
    return 0;

  for (i = 1; str[i]; i++) {
    // alphabet or number
    if (!isalnum(str[i]))
      break;
  }
  return i;
}

// append new token to cur and return pointer to new token
Token* new_token(Token *cur, TokenKind kind, char **str) {
  Token *next = (Token*)calloc(1, sizeof(Token));
  int len;
  if (kind == TK_IDENT)
    len = get_ident_len(*str);
  else
    len = TK_LEN_OF_KIND[kind];
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
  if (strncmp(p, "return", 6) == 0 && isspace(*(p+6))) {
    *kind = TK_RETURN;
    return true;
  }
  if (*p == '+') {
    *kind = TK_ADD;
    return true;
  }
  if (*p == '-') {
    *kind = TK_SUB;
    return true;
  }
  if (*p == '*') {
    *kind = TK_MUL;
    return true;
  }
  if (*p == '/') {
    *kind = TK_DIV;
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
  if (*p == ';') {
    *kind = TK_SEMICOLON;
    return true;
  }
  if (isalpha(*p)) {
    *kind = TK_IDENT;
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
// statement  = expr ";" | "return" expr ";"
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
  expect(TK_SEMICOLON);
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

  if (consume(TK_ASSIGN)) {
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
    if (consume(TK_EQ)) {
      rhs  = relational();
      node = new_node(ND_EQ, node, rhs);
    } else if (consume(TK_NE)) {
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
    if (consume(TK_LT)) {
      rhs  = add();
      node = new_node(ND_LT, node, rhs);
    } else if (consume(TK_LE)) {
      rhs  = add();
      node = new_node(ND_LE, node, rhs);
    } else if (consume(TK_GT)) {
      rhs  = add();
      node = new_node(ND_GT, node, rhs);
    } else if (consume(TK_GE)) {
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
    if (consume(TK_ADD)) {
      rhs  = mul();
      node = new_node(ND_ADD, node, rhs);
    } else if (consume(TK_SUB)) {
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
    if (consume(TK_MUL)) {
      rhs = unary();
      node = new_node(ND_MUL, node, rhs);
    } else if (consume(TK_DIV)) {
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
  
  if (consume(TK_ADD))
    return primary();
  if (consume(TK_SUB))
    return new_node(ND_SUB, new_node_number(0), primary());

  return primary();
}

// primary = num | ident | "(" expr ")"
Node* primary() {
  debug_put("primary\n");
  Node* node;
  // if '(' is found, 
  if (consume(TK_LPAR)) {
    node = expr();
    expect(TK_RPAR);   // ')' should be here
    return node;
  } else if (consume_ident(&node)) {
    // node = new_node(ND_IDENT, NULL, NULL);
    // node->offset = (id - 'a' + 1) * 8;
    return node;
  } else {
    return new_node_number(expect_number());
  }
}