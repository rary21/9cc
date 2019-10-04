#include "9cc.h"

const char* NODE_KIND_STR[NUM_NODE_KIND] =
  {"ND_ADD", "ND_SUB", "ND_MUL", "ND_DIV", "ND_NUM", "ND_LPAR", "ND_RPAR",
   "ND_EQ", "ND_NE", "ND_LT", "ND_LE", "ND_GT", "ND_GE", "ND_IDENT", "ND_ASSIGN", 
   "ND_RETURN", "ND_IF", "ND_WHILE", "ND_FOR", "ND_BLOCK", "ND_FUNC_CALL", "ND_FUNC_DEF",
   "ND_ADDR", "ND_DEREF"};

Node *prog[100];
LVar *locals;
Type *ptr_types[256];

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
Node* ident_def();

Node* new_node(NodeKind, Node*, Node*);

bool iseof() {
  return token == NULL || token->kind == TK_EOF;
}
bool istype() {
  return token->kind == TK_INT;
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
  node->type = new_type_int();
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

Node* new_node_func_def(char *func_name, Type *type) {
  debug_print("** new_node : %s\n", NODE_KIND_STR[ND_FUNC_DEF]);
  Node *node  = calloc(1, sizeof(Node));
  node->kind      = ND_FUNC_DEF;
  node->func_name = func_name;
  node->type      = type;
  return node;
}

Type* new_type(int ty, Type *ptr_to) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = ty;
  if (ty == PTR)
    type->ptr_to = ptr_to;
  if (ty == INT)
    type->size = 4;
  else if (ty == PTR) {
    type->size = 8;
  }
}

Type* new_type_int() {
  return new_type(INT, NULL);
}

int consume_ptrs() {
  int cnt = 0;
  while(token->kind == TK_MUL) {
    token = token->next;
    cnt++;
  }
  return cnt;
}

Type* consume_type() {
  if (!istype())
    error("no type found\n");

  token = token->next;
  int ptr_cnt = consume_ptrs();
  Type *type = calloc(1, sizeof(Type));
  if (ptr_types[ptr_cnt] == NULL) {
    int i_ptr;
    for (i_ptr = 0; i_ptr < ptr_cnt + 1; i_ptr++) {
      if (ptr_types[i_ptr] == NULL) {
        ptr_types[i_ptr] = calloc(1, sizeof(Type));
        if (i_ptr == 0) {
          ptr_types[i_ptr]->ty   = INT;
          ptr_types[i_ptr]->nptr = 0;
          ptr_types[i_ptr]->size = 4;
        } else {
          ptr_types[i_ptr]->ty     = PTR;
          ptr_types[i_ptr]->nptr   = i_ptr;
          ptr_types[i_ptr]->size   = 8;
          ptr_types[i_ptr]->ptr_to = ptr_types[i_ptr - 1];
        }
      }
    }
  }
  return ptr_types[ptr_cnt];
}

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

Node* consume_func_def() {
  if (!istype())
    error("function definition without return value type");
  Type *type = consume_type();
  if (token->kind == TK_IDENT) {
    char *name = calloc(1, 1 + token->len);
    strncpy(name, token->str, token->len);
    name[token->len] = '\0';
    Node *node = new_node_func_def(name, type);
    token = token->next;
    return node;
  }
  return NULL;
}

// go to next token and return name if current token is TK_IDENT
// otherwise, return NULL
void allocate_ident(Node** _node) {
  Node *node = *_node;
  LVar* lvar = find_lvar(node->name, strlen(node->name));
  if (lvar) {
    error("%s is already defined\n", node->name);
  } else {
    fprintf(stderr, "%s allocated size:%d\n", node->name, node->type->size);
    LVar* new_lvar   = (LVar*)calloc(1, sizeof(LVar));
    new_lvar->name   = node->name;
    new_lvar->next   = locals;
    new_lvar->len    = strlen(node->name);
    int offset = (node->type->ty == INT)? 4 : 8;
    if (locals)
      new_lvar->offset = locals->offset + offset;
    else
      new_lvar->offset = offset;
    if (node->type) {
      new_lvar->type = node->type;
    } else {
      error("no type found for %s\n", node->name);
    }
    locals           = new_lvar;
    node->offset = new_lvar->offset;
  }
}

void get_ident_info(Node** _node) {
  Node *node = *_node;
  LVar* lvar = find_lvar(node->name, strlen(node->name));
  if (!lvar)
    error("%s is not defined\n", node->name);
  node->offset = lvar->offset;
  node->type   = lvar->type;
}

Node* consume_ident() {
  if (token->kind == TK_IDENT) {
    Node *node = new_node(ND_IDENT, NULL, NULL);
    char *name = calloc(1, 1 + token->len);
    strncpy(name, token->str, token->len);
    name[token->len] = '\0';
    node->name = name;
    node->kind = ND_IDENT;
    token = token->next;
    return node;
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

// program    = func_def "{" statement* "}"
// statement  = expr ";" | "return" expr ";""
//              "if" "(" expr ")" statement ("else" statement)?
//              "while" "(" expr ")" statement
//              "for" "(" expr? ";" expr? ";" expr? ")" statement
//              "{" statement* "}"
//              ident_def
// ident_def  = "int" "*"* ident
// expr       = assignment
// assignment = equality ("=" assignment)*
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-" | "&" | "*")? primary
// primary    = num | ident ("(" expr* ")")? | "(" expr ")"
// func_def   = ident ("(" args_def ")") "{"
// args_def   = ident_def? ("," ident_def)*
void program() {
  int i = 0;
  Node *node, *func;

  while (func = func_def()) {
    if (!look_token(TK_LBRA))
      error("\"{\" Missing after function");
    func->body = statement();  // must be compound statement
    prog[i++] = func;
    locals = NULL;  // reset local variables
    debug_print("program: %d\n", i-1);
    if (iseof())
      break;
  }
  prog[i] = NULL;
}

// statement  = expr ";" | "return" expr ";""
//              "if" "(" expr ")" statement ("else" statement)?
//              "while" "(" expr ")" statement
//              "for" "(" expr? ";" expr? ";" expr? ")" statement
//              "{" statement* "}"
//              ident_def
Node* statement() {
  debug_put("statement\n");
  Node *node;
  if (consume(TK_RETURN)) {
    node = new_node(ND_RETURN, expr(), NULL);
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
  } else if (istype()) { // ident definition
    ident_def();         // currently, ident_def() does not output any assembly
    expect(TK_SEMICOLON);// ident definition ends with semicolon
    node = statement();
  } else {
    node = expr();
    expect(TK_SEMICOLON);
  }
  return node;
}

// ident_def  = "int" "*"* ident
Node* ident_def() {
  debug_put("ident_def\n");
  if (istype()) {
    Type *type  = consume_type();
    Node *ident = consume_ident();
    fprintf(stderr, "size:%d\n", type->size);
    ident->type = type;
    allocate_ident(&ident);
    return ident;
  } else {
    error("ident definition must starts with type\n");
  }
  return NULL;
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
  } else if (node = consume_ident()) {
    debug_print("%s found\n", node->name);
    if (consume(TK_LPAR)) {   // assuming function call
      node->func_name = node->name;  // copy to func_name
      node->kind      = ND_FUNC_CALL;
      node->type      = new_type_int();  // TODO: supoprt any return type
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
    } else {  // assuming ident
      get_ident_info(&node);
      fprintf(stderr, "node->name:%s %d\n", node->name, node->type->size);
    }
    return node;
  } else {
    return new_node_number(expect_number());
  }
}

// func_def   = type ident ("(" args_def ")") "{"
// args_def   = ident_def? ("," ident_def)*
Node* func_def() {
  debug_put("fnc_def\n");
  Node *func_def, *node;

  func_def = consume_func_def();

  if (func_def)
  {
    debug_print("function %s found\n", func_def->func_name);
    if (consume(TK_LPAR))
    { // assuming function call
      if (consume(TK_RPAR)) { // if no argument
        return func_def;
      }
      int i_arg = 0;
      // args_def   = ident_def? ("," ident_def)*
      while (i_arg < MAX_ARGS)
      {
        node = ident_def();
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