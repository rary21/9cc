#include "9cc.h"

const char* NODE_KIND_STR[NUM_NODE_KIND] =
  {"ND_ADD", "ND_SUB", "ND_MUL", "ND_DIV", "ND_NUM", "ND_EQ", "ND_NE", "ND_LT",
   "ND_LE", "ND_GT", "ND_GE", "ND_IDENT", "ND_ASSIGN", "ND_RETURN", "ND_IF",
   "ND_WHILE", "ND_FOR", "ND_BLOCK", "ND_FUNC_CALL", "ND_FUNC_DEF",
   "ND_ADDR", "ND_DEREF", "ND_SIZEOF", "ND_LVAR_DECL", "ND_GVAR_DECL",
   "ND_ARG_DECL", "ND_NONE"};

Node *prog[100];
LVar *locals;
Type *ptr_types[256];
Env  *env;

void program();
Node* top();
Node* statement();
Node* expr();
Node* assignment();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();
Node* func_def(Node *node, Type *type);
Node* ident_decl();

Node* new_node(NodeKind, Node*, Node*);
void allocate_ident(Node** _node);

bool iseof() {
  return token == NULL || token->kind == TK_EOF;
}
bool istype() {
  return token->kind == TK_INT;
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

Type* new_type_none() {
  Type *none = new_type(INT, NULL);
  none->size = 0;
  return none;
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

Node* new_node_func_def(char *func_name, Type *ret_type) {
  debug_print("** new_node : %s\n", NODE_KIND_STR[ND_FUNC_DEF]);
  Node *node      = calloc(1, sizeof(Node));
  node->kind      = ND_FUNC_DEF;
  node->func_name = func_name;
  node->name      = func_name;
  node->ret_type  = ret_type;
  Type *type      = new_type_none();
  node->type      = type;
  return node;
}

int consume_ptrs() {
  int cnt = 0;
  while(token->kind == TK_MUL) {
    token = token->next;
    cnt++;
  }
  return cnt;
}

int consume_type_name() {
  if (token->kind == TK_INT) {
    token = token->next;
    return INT;
  }
  error("unsupported type %s\n", TOKEN_KIND_STR[token->kind]);
}

int get_size(int ty) {
  if (ty == INT)
    return 4;
  if (ty == PTR)
    return 8;
  error("unsupported type %s\n", TOKEN_KIND_STR[token->kind]);
}

Type* consume_type() {
  if (!istype())
    return NULL;

  Type *type = calloc(1, sizeof(Type));
  type->ty   = consume_type_name();
  type->size = get_size(type->ty);
  int ptr_cnt = consume_ptrs();
  while (ptr_cnt > 0) {
    Type *new_type = calloc(1, sizeof(Type));
    new_type->ty     = PTR;
    new_type->size   = get_size(new_type->ty);
    new_type->ptr_to = type;

    type = new_type;

    ptr_cnt = ptr_cnt - 1;
  }
  return type;
}

Type* expect_type() {
  if (!istype())
    error("no type found\n");
  return consume_type();
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

LVar* new_locals() {
  LVar* lvar = calloc(1, sizeof(LVar));
  lvar->offset       = 0;
  lvar->len          = 0;
  lvar->name         = NULL;
  lvar->next         = NULL;
  lvar->type         = new_type_none();
  return lvar;
}

Env* new_env(Env *parent) {
  Env *env = calloc(1, sizeof(Env));
  env->parent = parent;
  env->locals = new_locals();
  return env;
}

// return LVar* which has same name, or NULL
LVar* find_lvar(const char* str, int len) {
  Env* _env;
  LVar* lvar;
  for (_env = env; _env; _env = _env->parent) {
    for (lvar = _env->locals; lvar; lvar = lvar->next) {
      // skip if name length is diffrent
      if (lvar->len != len)
        continue;
      if (strncmp(lvar->name, str, len) == 0)
        return lvar;
    }
  }
  return NULL;
}

// return LVar* which has same name, or NULL only in current block
LVar* find_lvar_current_block(const char* str, int len) {
  Env* _env;
  LVar* lvar;
  for (lvar = env->locals; lvar; lvar = lvar->next) {
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
void allocate_ident(Node** _node) {
  Node *node = *_node;
  LVar* lvar = find_lvar_current_block(node->name, strlen(node->name));
  if (lvar) {
    error("%s is already defined\n", node->name);
  } else {
    LVar* new_lvar   = (LVar*)calloc(1, sizeof(LVar));
    debug_print("%s allocated size:%d %p\n", node->name, node->type->size, new_lvar);
    new_lvar->name   = node->name;
    new_lvar->next   = env->locals;
    new_lvar->len    = strlen(node->name);

    if (node->type)
      new_lvar->type = node->type;
    else
      error("no type found for %s\n", node->name);
    
    node->var   = new_lvar;
    env->locals = new_lvar;
  }
}

void get_ident_info(Node** _node) {
  Node *node = *_node;
  LVar* lvar = find_lvar(node->name, strlen(node->name));
  if (!lvar)
    error("%s is not defined\n", node->name);
  node->var  = lvar;
  node->type = lvar->type;
  debug_print("getidentinfo %p\n", lvar);
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

Node* expect_lvar_decl() {
  if (token->kind == TK_IDENT) {
    Node *node = new_node(ND_IDENT, NULL, NULL);
    char *name = calloc(1, 1 + token->len);
    strncpy(name, token->str, token->len);
    name[token->len] = '\0';
    node->name = name;
    node->kind = ND_LVAR_DECL;
    token = token->next;
    return node;
  } else {
    error("expect identifier but got %s\n", TOKEN_KIND_STR[token->kind]);
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
bool look_token(TokenKind kind, int count) {
  Token *tkn = token;
  while (count > 0) {
    tkn = tkn->next;
    count = count - 1;
  }
  return tkn->kind == kind;
}

// program    = top*
// top        = func_def | iden_decl
// statement  = expr ";" | "return" expr ";""
//              "if" "(" expr ")" statement ("else" statement)?
//              "while" "(" expr ")" statement
//              "for" "(" expr? ";" expr? ";" expr? ")" statement
//              "{" statement* "}"
//              ident_decl
// ident_decl = "int" "*"* ident
// expr       = assignment
// assignment = equality ("=" assignment)*
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-" | "&" | "*")? primary
// primary    = num | ident ("(" expr* ")")? | "(" expr ")" | sizeof(unary)
// func_def   = ident ("(" args_def ")") "{"
// args_def   = ident_decl? ("," ident_decl)*
void program() {
  int i = 0;
  Node *node;

  env    = new_env(NULL);
  while (node = top()) {
    prog[i++] = node;
    if (iseof())
      break;
  }
  prog[i] = NULL;
}

Node* top() {
  Node *node;
  Type *type;
  type = expect_type();
  node = consume_ident();
  if (look_token(TK_LPARE, 0)) {   // function definition
    node = func_def(node, type);
  } else if (consume(TK_LBBRA)) {  // global array variable
    node->kind       = ND_GVAR_DECL;
    Type *deref_type = new_type(type->ty, type->ptr_to);
    type->ptr_to     = deref_type;
    type->ty         = ARRAY;
    type->array_size = expect_number();
    type->size       = type->array_size * type->size;
    type->is_global  = true;
    expect(TK_RBBRA);
    expect(TK_SEMICOLON);
  } else {
    node->kind       = ND_GVAR_DECL;
    type->is_global  = true;
    expect(TK_SEMICOLON);
  }
  node->type = type;
  allocate_ident(&node);

  return node;
}

// statement  = expr ";" | "return" expr ";""
//              "if" "(" expr ")" statement ("else" statement)?
//              "while" "(" expr ")" statement
//              "for" "(" expr? ";" expr? ";" expr? ")" statement
//              "{" statement* "}"
//              ident_decl
Node* statement() {
  debug_put("statement\n");
  Node *node;
  if (consume(TK_RETURN)) {
    node = new_node(ND_RETURN, expr(), NULL);
    expect(TK_SEMICOLON);
  } else if (consume(TK_IF)) {   // if statement
    expect(TK_LPARE);
    Node *condition = expr();
    expect(TK_RPARE);
    node = new_node_if(condition, statement(), NULL);
    if (consume(TK_ELSE))
      node->else_statement = statement();
  } else if (consume(TK_WHILE)) {  // while statement
    expect(TK_LPARE);
    Node *condition = expr();
    expect(TK_RPARE);
    node = new_node_while(condition, statement());
  } else if (consume(TK_FOR)) {  // for statement
    Node *init      = NULL;
    Node *condition = NULL;
    Node *last      = NULL;
    expect(TK_LPARE);
    env = new_env(env);
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
    expect(TK_RPARE);
    env = env->parent;
    node = new_node_for(condition, init, last, statement());
  } else if (consume(TK_LCBRA)) {  // block
    int i_block = 0;
    node = new_node(ND_BLOCK, NULL, NULL);
    env = new_env(env);
    while (!consume(TK_RCBRA))
      node->block[i_block++] = statement();
    env = env->parent;
    node->block[i_block] = NULL;
  } else if (istype()) { // ident declaration
    node = ident_decl(); // ident declaration will be used in semantic analysys
    expect(TK_SEMICOLON);// ident declaration ends with semicolon
  } else {
    node = expr();
    expect(TK_SEMICOLON);
  }
  return node;
}

// ident_decl  = "int" "*"* ident ("[" num "]")?
Node* ident_decl() {
  debug_put("ident_decl\n");
  if (istype()) {
    Type *type  = consume_type();
    Node *ident = expect_lvar_decl();
    if (consume(TK_LBBRA)) {
      Type *deref_type = new_type(type->ty, type->ptr_to);
      type->ptr_to     = deref_type;
      type->ty         = ARRAY;
      type->array_size = expect_number();
      type->size       = type->array_size * type->size;
      expect(TK_RBBRA);
    }
    debug_print("size:%d\n", type->size);
    ident->type = type;
    allocate_ident(&ident);
    debug_put("alloc end\n");
    return ident;
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

// primary    = num | ident ("(" expr* ")")? | "(" expr ")" | sizeof(unary)
//              ident "[" expr "]"
Node* primary() {
  debug_put("primary\n");
  Node* node;
  char* name;
  // if '(' is found, 
  if (consume(TK_LPARE)) {
    node = expr();
    expect(TK_RPARE);   // ')' should be here
    return node;
  } else if (consume(TK_SIZEOF)) {
    if (consume(TK_LPARE)) {
      node = new_node(ND_SIZEOF, unary(), NULL);
      expect(TK_RPARE);
    } else {
      node = new_node(ND_SIZEOF, unary(), NULL);
    }
    return node;
  } else if (node = consume_ident()) {
    debug_print("%s found\n", node->name);
    if (consume(TK_LPARE)) {   // assuming function call
      node->func_name = node->name;  // copy to func_name
      node->kind      = ND_FUNC_CALL;
      node->type      = new_type_int();  // TODO: supoprt any return type
      if (consume(TK_RPARE)) { // if no argument
        return node;
      }
      int i_arg = 0;
      // args = expr ("," expr)*
      while (i_arg < MAX_ARGS) {
        node->args_call[i_arg++] = expr();
        if (consume(TK_RPARE))
          break;
        expect(TK_COMMA);
      }
      node->args_call[i_arg] = NULL;
    } else if (consume(TK_LBBRA)) {  // assuming array accsess
      get_ident_info(&node);
      Node *add   = new_node(ND_ADD, node, expr());
      Node *deref = new_node(ND_DEREF, add, NULL);
      expect(TK_RBBRA);
      return deref;
    } else {  // assuming ident
      get_ident_info(&node);
      debug_print("node->name:%s %d\n", node->name, node->var->type->size);
    }
    return node;
  } else {
    return new_node_number(expect_number());
  }
}

// func_def   = type ident ("(" args_def ")") "{"
// args_def   = ident_decl? ("," ident_decl)*
Node* func_def(Node *func_def, Type *ret_type) {
  debug_put("fnc_def\n");
  Node *node;

  func_def->kind      = ND_FUNC_DEF;
  func_def->func_name = func_def->name;
  func_def->ret_type  = ret_type;
  Type *type      = new_type_none();

  if (func_def)
  {
    debug_print("function %s found\n", func_def->func_name);
    if (consume(TK_LPARE))
    { // assuming function call
      int i_arg = 0;
      env = new_env(env);
      // args_def   = ident_decl? ("," ident_decl)*
      while (i_arg < MAX_ARGS)
      {
        node = ident_decl();
        if (!node && consume(TK_RPARE))
          break;
        node->kind = ND_ARG_DECL;
        func_def->args_def[i_arg++] = node;
        if (consume(TK_RPARE))
          break;
        expect(TK_COMMA);
      }
      func_def->args_def[i_arg] = NULL;
      if (!look_token(TK_LCBRA, 0))
        error("function definition must be with {\n");
      func_def->body = statement(); // must be compound statement
      env = env->parent;
      return func_def;
    }
  }

  return NULL;
}