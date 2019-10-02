#include "9cc.h"

const char* NODE_KIND_STR[NUM_NODE_KIND] =
  {"ND_ADD", "ND_SUB", "ND_MUL", "ND_DIV", "ND_NUM", "ND_LPAR", "ND_RPAR",
   "ND_EQ", "ND_NE", "ND_LT", "ND_LE", "ND_GT", "ND_GE", "ND_IDENT", "ND_ASSIGN", 
   "ND_RETURN", "ND_IF", "ND_WHILE", "ND_FOR", "ND_BLOCK", "ND_FUNC_CALL", "ND_FUNC_DEF",
   "ND_ADDR", "ND_DEREF"};

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
Node* func_def();

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

// go to next token and return name if current token is TK_IDENT
// otherwise, return NULL
char* allocate_ident(Node** node) {
  if (token->kind == TK_IDENT) {
    *node = new_node(ND_IDENT, NULL, NULL);
    LVar* lvar = find_lvar(token->str, token->len);
    if (lvar) {
      (*node)->offset = lvar->offset;
      token = token->next;
      return lvar->name;
    } else {
      LVar* new_lvar   = (LVar*)calloc(1, sizeof(LVar));
      new_lvar->name   = calloc(1, 1 + token->len);
      strncpy(new_lvar->name, token->str, token->len);
      new_lvar->name[token->len] = '\0';
      new_lvar->len    = token->len;
      new_lvar->next   = locals;
      if (locals)
        new_lvar->offset = locals->offset + 8;
      else
        new_lvar->offset = 8;
      locals           = new_lvar;
      (*node)->offset = new_lvar->offset;
      token = token->next;
      return new_lvar->name;
    }
  }
  return NULL;
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

// return true if current token is kind
bool look_token(TokenKind kind) {
  return token->kind == kind;
}

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
  debug_print("** new_node : %s\n", NODE_KIND_STR[kind]);
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs  = lhs;
  node->rhs  = rhs;
  return node;
}

Node* new_node_number(int val) {
  debug_print("** new_node_number : %d\n", val);
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val  = val;
  return node;
}

Node* new_node_if(Node* condition, Node* if_statement, Node* else_statement) {
  static int if_cnt = 0;
  debug_print("** new_node : %s\n", NODE_KIND_STR[ND_IF]);
  Node *node  = calloc(1, sizeof(Node));
  node->kind           = ND_IF;
  node->condition      = condition;
  node->if_statement   = if_statement;
  node->else_statement = else_statement;
  sprintf(node->label_s, ".LIF_ELSE%d", if_cnt++);
  sprintf(node->label_e, ".LIF_END%d", if_cnt++);
  return node;
}

Node* new_node_while(Node* condition, Node* statement) {
  static int while_cnt = 0;
  debug_print("** new_node : %s\n", NODE_KIND_STR[ND_WHILE]);
  Node *node = calloc(1, sizeof(Node));
  node->kind      = ND_WHILE;
  node->condition = condition;
  node->statement = statement;
  sprintf(node->label_s, ".LWHILE_BEGIN%d", while_cnt++);
  sprintf(node->label_e, ".LWHILE_END%d", while_cnt++);
  return node;
}

Node* new_node_for(Node* condition, Node* init, Node* last, Node* statement) {
  static int while_cnt = 0;
  debug_print("** new_node : %s\n", NODE_KIND_STR[ND_FOR]);
  Node *node  = calloc(1, sizeof(Node));
  node->kind      = ND_FOR;
  node->init      = init;
  node->last      = last;
  node->condition = condition;
  node->statement = statement;
  sprintf(node->label_s, ".LFOR_BEGIN%d", while_cnt++);
  sprintf(node->label_e, ".LFOR_END%d", while_cnt++);
  return node;
}

Node* new_node_func_call(char* func_name) {
  debug_print("** new_node : %s\n", NODE_KIND_STR[ND_FUNC_CALL]);
  Node *node  = calloc(1, sizeof(Node));
  node->kind      = ND_FUNC_CALL;
  node->func_name = func_name;
  return node;
}

Node* new_node_func_def(char* func_name) {
  debug_print("** new_node : %s\n", NODE_KIND_STR[ND_FUNC_DEF]);
  Node *node  = calloc(1, sizeof(Node));
  node->kind      = ND_FUNC_DEF;
  node->func_name = func_name;
  return node;
}

// program    = func_def "{" statement* "}"
// statement  = expr ";" | "return" expr ";""
//              "if" "(" expr ")" statement ("else" statement)?
//              "while" "(" expr ")" statement
//              "for" "(" expr? ";" expr? ";" expr? ")" statement
//              "{" statement* "}"
// expr       = assignment
// assignment = equality ("=" assignment)*
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-" | "&" | "*")? primary
// primary    = num | ident ("(" expr* ")")? | "(" expr ")"
// func_def   = ident ("(" args_def ")") "{"
// args_def   = ident? ("," ident)*
void program() {
  int i = 0;
  Node *node, *func;

  while (func = func_def()) {
    if (!look_token(TK_LBRA))
      error("\"{\" Missing after function");
    func->statement = statement();  // must be compound statement
    prog[i++] = func;
    locals = NULL;  // reset local variables
  }
  prog[i] = NULL;
}

// statement  = expr ";" | "return" expr ";""
//              "if" "(" expr ")" statement ("else" statement)?
//              "while" "(" expr ")" statement
//              "for" "(" expr? ";" expr? ";" expr? ")" statement
//              "{" statement* "}"
Node* statement() {
  debug_put("statement\n");
  Node *node;
  if (consume(TK_RETURN)) {
    node = new_node(ND_RETURN, NULL, expr());
    expect(TK_SEMICOLON);
  } else if (consume(TK_IF)) {   // if statement
    expect(TK_LPAR);
    Node *condition = expr();
    expect(TK_RPAR);
    node = new_node_if(condition, statement(), NULL);
    if (consume(TK_ELSE))
      node->else_statement = statement();
  } else if (consume(TK_WHILE)) {  // while statement
    expect(TK_LPAR);
    Node *condition = expr();
    expect(TK_RPAR);
    node = new_node_while(condition, statement());
  } else if (consume(TK_FOR)) {  // for statement
    Node *init      = NULL;
    Node *condition = NULL;
    Node *last      = NULL;
    expect(TK_LPAR);
    if (!consume(TK_SEMICOLON)) {
      init = expr();
      expect(TK_SEMICOLON);
    }
    if (!consume(TK_SEMICOLON)) {
      condition = expr();
      expect(TK_SEMICOLON);
    }
    if (!consume(TK_SEMICOLON)) {
      last = expr();
    }
    expect(TK_RPAR);
    node = new_node_for(condition, init, last, statement());
  } else if (consume(TK_LBRA)) {  // block
    int i_block = 0;
    node = new_node(ND_BLOCK, NULL, NULL);
    while (!consume(TK_RBRA))
      node->block[i_block++] = statement();
    node->block[i_block] = NULL;
  } else {
    node = expr();
    expect(TK_SEMICOLON);
  }
  return node;
}

// expr       = assignment
Node* expr() {
  debug_put("expr\n");
  return assignment();
}

// assignment = equality ("=" assignment)*
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

// unary      = ("+" | "-" | "&" | "*")? primary
Node* unary() {
  debug_put("unary\n");
  
  if (consume(TK_ADD))
    return primary();
  if (consume(TK_SUB))
    return new_node(ND_SUB, new_node_number(0), primary());
  if (consume(TK_AND))
    return new_node(ND_ADDR, primary(), NULL);
  if (consume(TK_MUL))
    return new_node(ND_DEREF, primary(), NULL);

  return primary();
}

// primary    = num | ident ("(" expr* ")")? | "(" expr ")"
Node* primary() {
  debug_put("primary\n");
  Node* node;
  char* name;
  // if '(' is found, 
  if (consume(TK_LPAR)) {
    node = expr();
    expect(TK_RPAR);   // ')' should be here
    return node;
  } else if (name = allocate_ident(&node)) {  // TODO: avoid allocation for function
    debug_print("%s found\n", name);
    if (consume(TK_LPAR)) {   // assuming function call
      node = new_node_func_call(name);
      if (consume(TK_RPAR)) { // if no argument
        return node;
      }
      int i_arg = 0;
      // args = expr ("," expr)*
      while (i_arg < MAX_ARGS) {
        node->args_call[i_arg++] = expr();
        if (consume(TK_RPAR))
          break;
        expect(TK_COMMA);
      }
      node->args_call[i_arg] = NULL;
    }
    return node;
  } else {
    return new_node_number(expect_number());
  }
}

// func_def   = ident ("(" args_def ")") "{"
// args_def   = ident? ("," ident)*
Node* func_def() {
  debug_put("fnc_def\n");
  Node* func_def, *node;
  char* name;

  if (name = allocate_ident(&node)) // TODO: avoid allocation for function
  {
    debug_print("%s found\n", name);
    if (consume(TK_LPAR))
    { // assuming function call
      func_def = new_node_func_def(name);
      if (consume(TK_RPAR)) { // if no argument
        return func_def;
      }
      int i_arg = 0;
      // args_def   = ident? ("," ident)*
      while (i_arg < MAX_ARGS)
      {
        name = allocate_ident(&node);
        if (!name)
          break;
        func_def->args_def[i_arg++] = node;
        if (consume(TK_RPAR))
          break;
        expect(TK_COMMA);
      }
      func_def->args_def[i_arg] = NULL;
      if (!look_token(TK_LBRA))  // function definition needs "{"
        error("function definition expected, but not \"{\" found");
      return func_def;
    }
  }

  return NULL;
}