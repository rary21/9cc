#include "9cc.h"

const char* NODE_KIND_STR[NUM_NODE_KIND] =
  {"ND_ADD", "ND_SUB", "ND_MUL", "ND_DIV", "ND_NUM", "ND_EQ", "ND_NE", "ND_LT",
   "ND_LE", "ND_GT", "ND_GE", "ND_IDENT", "ND_LITERAL", "ND_ASSIGN", "ND_VAR_INIT", "ND_RETURN",
   "ND_IF", "ND_WHILE", "ND_FOR", "ND_BLOCK", "ND_FUNC_CALL", "ND_FUNC_DECL", "ND_FUNC_DEF",
   "ND_ADDR", "ND_DEREF", "ND_DOT", "ND_ARROW", "ND_CAST", "ND_SIZEOF", "ND_LVAR_DECL",
   "ND_LVAR_INIT", "ND_GVAR_DECL", "ND_ARG_DECL", "ND_POSTINC", "ND_NONE"};

typedef struct Env Env;
struct Env {
  Vector *locals;
  Map *struct_types;
  Map *typedefs;
  Env *parent;
};

Node *prog[100];
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
static Node* cast();
Node* unary();
Node* postfix();
Node* primary();
Node* func_decl_def(Node *node, Type *type);
Node* ident_init();
Node* ident_decl();
Node* decl_spec();
Map *struct_members();
Type* find_struct_type(const char* str);
Type* find_typedef(const char* str);

Node* new_node(NodeKind, Node*, Node*);
void allocate_ident(Node** _node);

bool iseof() {
  Token *token = vector_get_front(vec_token);
  return token == NULL || token->kind == TK_EOF;
}
bool istype() {
  Token *token = vector_get_front(vec_token);
  return token->kind == TK_INT || token->kind == TK_CHAR || token->kind == TK_STRUCT ||
         token->kind == TK_CONST;
}
bool istype_token(Token *token) {
  return token->kind == TK_INT || token->kind == TK_CHAR || token->kind == TK_STRUCT ||
         token->kind == TK_CONST;
}
bool in_typedefs(char *str) {
  Env* _env;
  for (_env = env; _env; _env = _env->parent) {
      Type *type = map_find(_env->typedefs, str);
      if (type)
        return true;
  }
  return false;
}
bool is_type_spec() {
  Token *token = vector_get_front(vec_token);
  return token->kind == TK_INT || token->kind == TK_CHAR || token->kind == TK_STRUCT ||
         in_typedefs(token->str);
}
bool is_type_qual() {
  Token *token = vector_get_front(vec_token);
  return token->kind == TK_CONST;
}
bool isstorage() {
  Token *token = vector_get_front(vec_token);
  return token->kind == TK_STATIC || token->kind == TK_TYPEDEF;
}
bool is_type_or_storage() {
  return istype() || isstorage();
}

static bool consume(TokenKind kind) {
  Token *token = vector_get_front(vec_token);
  if (token->kind != kind)
    return false;
  // next token
  debug_print("** consuming token->kind: %s\n",
               TOKEN_KIND_STR[token->kind]);
  vector_pop_front(vec_token);
  return true;
}

// go to next token and return true if current token has same kind
// otherwise, throw error and exit application
static void expect(TokenKind kind) {
  Token *token = vector_pop_front(vec_token);
  if (token->kind != kind)
    error("LINE: %d %s expected, but got %s", get_line_number(token),
          TOKEN_KIND_STR[kind], TOKEN_KIND_STR[token->kind]);
}

Type* new_type(int ty, Type *ptr_to) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = ty;
  if (ty == PTR)
    type->ptr_to = ptr_to;

  if (ty == INT) {
    type->size  = 4;
    type->align = 4;
  } else if (ty == CHAR) {
    type->size  = 1;
    type->align = 1;
  } else if (ty == PTR) {
    type->size  = 8;
    type->align = 8;
  } else if (ty == STRUCT) {
    type->size  = 0;
    type->align = 8;
  }
  // const qualifier should be preserved
  if (ptr_to)
    type->is_const = ptr_to->is_const;

  return type;
}

Type* new_type_int() {
  return new_type(INT, NULL);
}

Type* new_type_char() {
  return new_type(CHAR, NULL);
}

Type* new_type_struct() {
  return new_type(STRUCT, NULL);
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

Node* new_node_post_inc(Node* _node, int _inc) {
  debug_put("** new_node : post_inc\n");

  if (_node->kind != ND_IDENT)
    error("post_inc got not IDENT");
  Node *node      = new_node(ND_POSTINC, NULL, NULL);
  Node *add       = new_node(ND_ADD, _node, new_node_number(_inc));
  Node *inc       = new_node(ND_ASSIGN, _node, add);
  node->rhs       = _node;
  node->statement = inc;

  return node;
}

Node* consume_ident() {
  Token *token = vector_get_front(vec_token);
  if (token->kind == TK_IDENT) {
    Node *node = new_node(ND_IDENT, NULL, NULL);
    char *name = calloc(1, 1 + token->len);
    strncpy(name, token->str, token->len);
    name[token->len] = '\0';
    node->name  = name;
    node->len   = token->len;
    node->kind  = ND_IDENT;
    node->token = token;
    token = vector_pop_front(vec_token);
    return node;
  }
  return NULL;
}

Node* expect_ident() {
  Token *token = vector_get_front(vec_token);
  if (token->kind != TK_IDENT)
    error("ident expected");
  return consume_ident();
}

Node* consume_literal() {
  Token *token = vector_get_front(vec_token);
  if (token->kind == TK_LITERAL) {
    Node *node = new_node(ND_LITERAL, NULL, NULL);
    char *name = calloc(1, 1 + token->len);
    strncpy(name, token->str, token->len);
    name[token->len] = '\0';
    node->name         = name;
    node->kind         = ND_LITERAL;
    node->type         = new_type(PTR, NULL);
    node->token        = token;
    node->literal_id   = token->literal_id;
    token = vector_pop_front(vec_token);
    return node;
  }
  return NULL;
}

int consume_ptrs() {
  int cnt = 0;
  while(1) {
    Token *token = vector_get_front(vec_token);
    if (token->kind != TK_MUL)
      break;
    vector_pop_front(vec_token);
    cnt++;
  }
  return cnt;
}

void consume_type_name(Type *type) {
  Token *token = vector_pop_front(vec_token);
  if (token->kind == TK_INT) {
    type->ty = INT;
    type->size = 4;
    type->align = 4;
  } else if (token->kind == TK_CHAR) {
    type->ty = CHAR;
    type->size = 1;
    type->align = 1;
  } else if (token->kind == TK_STRUCT) {
    type->ty = STRUCT;
    type->size = 0;
    type->align = 8;
  } else if (token->kind == TK_IDENT) {
    type = find_typedef(token->str);
  } else {
    error("unsupported type %s\n", TOKEN_KIND_STR[token->kind]);
  }
}

int get_size(int ty) {
  Token *token = vector_get_front(vec_token);
  if (ty == INT)
    return 4;
  if (ty == CHAR)
    return 1;
  if (ty == PTR)
    return 8;
  if (ty == STRUCT)
    return 0;
  error("unsupported type %s\n", TOKEN_KIND_STR[token->kind]);
}

void struct_fix_alignment(Type *type) {
  Map *members = type->members;
  int size = 0;
  debug_print("member count : %d\n", members->len);
  for (int i = 0; i < members->len; i++) {
    Type *member = vector_get(members->vals, i);
    member->offset = roundup(size, member->align);
    size = member->offset + member->size;
  }
  type->size = roundup(size, 4);
  debug_print("struct size : %d\n", type->size);
}

void add_struct_type(char *name, Type *type) {
  map_add(env->struct_types, name, type);
}

void copy_type_info(Type *to, Type *from) {
  to->ty = from->ty;
  to->ptr_to = from->ptr_to;
  to->size = from->size;
  to->align = from->align;
  to->array_size = from->array_size;
  to->offset = from->offset;
  to->members = from->members;
  to->name = from->name;
}

bool consume_type_spec(Type *type) {
  if (type->ty != INVALID)
    error("type_spec is already specified for this delcaration");
  if (!is_type_spec())
    return false;
  consume_type_name(type);
  if (type->ty == STRUCT) {
    Node *ident = expect_ident();
    if (consume(TK_LCBRA)) { // member definition
      type->members = struct_members();
      type->name = ident->name;
      struct_fix_alignment(type);
      add_struct_type(ident->name, type);
    } else { // otherwise, struct already defined
      Type *_type = find_struct_type(ident->name);
      copy_type_info(type, _type);
    }
  }
  return true;
}

bool consume_type_qual(Type *type) {
  if (!is_type_qual())
    return false;

  Token *token = vector_pop_front(vec_token);
  if (token->kind == TK_CONST)
    type->is_const = true;
  return true;
}

bool consume_storage_spec(Type *type) {
  if (!isstorage())
    return false;

  Token *token = vector_pop_front(vec_token);
  if (token->kind == TK_STATIC)
    type->is_static = true;
  if (token->kind == TK_TYPEDEF)
    type->is_typedef = true;
  return true;
}

Type* consume_decl_spec_ptr() {
  if (!is_type_or_storage())
    return NULL;

  Type *type = calloc(1, sizeof(Type));
  type->ty   = INVALID;
  while (is_type_or_storage()) {
    consume_type_spec(type);
    consume_type_qual(type);
    consume_storage_spec(type);
  }
  int ptr_cnt = consume_ptrs();

  while (ptr_cnt > 0) {
    type = new_type(PTR, type);
    ptr_cnt = ptr_cnt - 1;
  }
  return type;
}

Type* expect_decl_spec_ptr() {
  if (!is_type_or_storage())
    error("no type found\n");
  return consume_decl_spec_ptr();
}

// go to next token and return true if current token has same kind
// otherwise, return false
static Env* new_env(Env *parent) {
  Env *env = calloc(1, sizeof(Env));
  env->parent       = parent;
  env->locals       = new_vector();
  env->struct_types = new_map();
  env->typedefs     = new_map();
  return env;
}

// return LVar* which has same name, or NULL
LVar* find_lvar(const char* str, int len) {
  Env* _env;
  for (_env = env; _env; _env = _env->parent) {
    for (int i = 0; i < _env->locals->len; i++) {
      LVar *lvar = vector_get(_env->locals, i);
      // skip if name length is diffrent
      if (lvar->len != len)
        continue;
      if (strncmp(lvar->name, str, len) == 0)
        return lvar;
    }
  }
  return NULL;
}

// return struct types which has same name, or NULL
Type* find_struct_type(const char* str) {
  Env* _env;
  for (_env = env; _env; _env = _env->parent) {
      Type *type = map_find(_env->struct_types, str);
      if (type)
        return type;
  }
  error("struct %s not found", str);
}

Type* find_typedef(const char* str) {
  Env* _env;
  for (_env = env; _env; _env = _env->parent) {
      Type *type = map_find(_env->typedefs, str);
      if (type)
        return type;
  }
  error("typedef %s not found", str);
}

// return LVar* which has same name, or NULL only in current block
LVar* find_lvar_current_block(const char* str, int len) {
  for (int i = 0; i < env->locals->len; i++) {
    LVar *lvar = vector_get(env->locals, i);
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
    new_lvar->len    = strlen(node->name);

    if (node->type)
      new_lvar->type = node->type;
    else
      error("no type found for %s\n", node->name);
    
    node->var   = new_lvar;

    if (!env->parent) {
      debug_print("global %s\n", node->name);
      new_lvar->is_global = true;
    } else {
      debug_print("local %s\n", node->name);
      new_lvar->is_global = false;
    }

    vector_push_back(env->locals, new_lvar);
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

Node* expect_lvar_decl() {
  Token *token = vector_get_front(vec_token);
  if (token->kind == TK_IDENT) {
    Node *node = new_node(ND_IDENT, NULL, NULL);
    char *name = calloc(1, 1 + token->len);
    strncpy(name, token->str, token->len);
    name[token->len] = '\0';
    node->name  = name;
    node->kind  = ND_LVAR_DECL;
    node->token = token;
    token = vector_pop_front(vec_token);
    return node;
  } else {
    error("expect identifier but got %s\n", TOKEN_KIND_STR[token->kind]);
  }
  return NULL;
}

// go to next token and return value if current token is TK_NUM
// otherwise, throw error and exit application
int expect_number() {
  Token *token = vector_get_front(vec_token);
  if (token->kind != TK_NUM)
    error("number expected, but got %s", TOKEN_KIND_STR[token->kind]);
  // next token
  int value = token->val;
  token = vector_pop_front(vec_token);
  return value;
}

// return true if current token is kind
bool look_token(TokenKind kind, int count) {
  Token *tkn;
  tkn = vec_token->elem[vec_token->index + count];

  return tkn->kind == kind;
}

// return true if current token is kind
Token* get_token(int count) {
  return vec_token->elem[vec_token->index + count];
}

// program        = top*
// top            = func_decl_def | iden_decl
// statement      = expr ";" | "return" expr ";""
//                  "if" "(" expr ")" statement ("else" statement)?
//                  "while" "(" expr ")" statement
//                  "for" "(" expr? ";" expr? ";" expr? ")" statement
//                  "{" statement* "}"
//                  ident_init
// ident_init     = ident_decl ("=" assignment)?
// ident_decl     = decl_spec pointer ident ("[" num "]")?
// param_decl     = spec_qual pointer ident ("[" num "]")?
// decl_spec      = (storage_spec | type_spec | type_qual)*
// spec_qual      = (type_spec | type_qual)*
// type_spec      = int | char | struct_spec
// type_qual      = const
// pointer        = "*"*
// struct_spec    = struct ident
// storage_spec   = typedef
// expr           = assignment
// assignment     = equality ("=" assignment)*
// equality       = relational ("==" relational | "!=" relational)*
// relational     = add ("<" add | "<=" add | ">" add | ">=" add)*
// add            = mul ("+" mul | "-" mul)*
// mul            = cast ("*" cast | "/" cast)*
// cast           = unary | (spec_qual pointer) cast
// unary          = postfix
//                  |("++" | "--")? unary
//                  | unary_op cast
// unary_op       = ("+" | "-" | "&" | "*")?
// postfix        = primary
//                  | primary ("++" | "--")
//                  | primary "[" expr "]"
//                  | primary "(" args_def ")"
// primary        = num | ident ("(" expr* ")")? | "(" expr ")" | sizeof(unary)
//                  ident "[" expr "]" | \"characters\"
// func_decl_def  = func_decl | func_def
// func_decl      = type ident ("(" args_def ")") ";"
// args_def       = param_decl? ("," param_decl)*
// struct_members = param_decl? (";" param_decl)*
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
  type = expect_decl_spec_ptr();
  node = consume_ident();
  if (look_token(TK_LPARE, 0)) {   // function definition
    node = func_decl_def(node, type);
  } else if (consume(TK_LBBRA)) {  // global array variable
    node->kind       = ND_GVAR_DECL;
    Type *deref_type = new_type(type->ty, type->ptr_to);
    type->ptr_to     = deref_type;
    type->ty         = ARRAY;
    type->array_size = expect_number();
    type->size       = type->array_size * type->size;
    debug_print("arraysize %d\n", type->size);
    expect(TK_RBBRA);
    expect(TK_SEMICOLON);
    node->type = type;
    allocate_ident(&node);
  } else if (node) {
    node->kind       = ND_GVAR_DECL;
    expect(TK_SEMICOLON);
    node->type = type;
    allocate_ident(&node);
  } else { // struct type declaration
    expect(TK_SEMICOLON);
    node = calloc(1, sizeof(Node));
    node->kind = ND_NONE;
  }

  return node;
}

// statement  = expr ";" | "return" expr ";""
//              "if" "(" expr ")" statement ("else" statement)?
//              "while" "(" expr ")" statement
//              "for" "(" expr? ";" expr? ";" expr? ")" statement
//              "{" statement* "}"
//              ident_init
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
  } else if (is_type_or_storage()) { // ident declaration
    node = ident_init();  // ident declaration will be used in semantic analysys
    expect(TK_SEMICOLON);// ident declaration ends with semicolon
  } else {
    node = expr();
    expect(TK_SEMICOLON);
  }
  return node;
}

// ident_init  = ident_decl ("=" assignment)?
Node* ident_init() {
  debug_put("ident_init\n");
  Node *ident = ident_decl();
  allocate_ident(&ident);
  if (consume(TK_ASSIGN)) {
    Node *lvar = new_node(ND_IDENT, NULL, NULL);
    lvar->name = ident->name;
    get_ident_info(&lvar);
    Node *init = expr();
    Node *assign = new_node(ND_VAR_INIT, lvar, init);
    Node *node   = new_node(ND_LVAR_INIT, ident, assign);
    return node;
  }
  return ident;
}

// ident_decl  = decl_spec "*"* ident ("[" num "]")?
Node* ident_decl() {
  debug_put("ident_decl\n");
  if (is_type_or_storage()) {
    Type *type  = consume_decl_spec_ptr();
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

// mul         = cast ("*" cast | "/" cast)*
Node* mul() {
  debug_put("mul\n");
  Node *node = cast();
  Node *rhs;

  while (1) {
    if (consume(TK_MUL)) {
      rhs = cast();
      node = new_node(ND_MUL, node, rhs);
    } else if (consume(TK_DIV)) {
      rhs = cast();
      node = new_node(ND_DIV, node, rhs);
    } else {
      return node;
    }
  }
}

// cast        = unary | (spec_qual pointer) cast
static Node* cast() {
  debug_put("cast\n");
  Node *node;
  Token *cur  = get_token(0);
  Token *next = get_token(1);

  while (1) {
    if (cur->kind == TK_LPARE && istype_token(next)) {
      consume(TK_LPARE);
      Type *type = consume_decl_spec_ptr();
      expect(TK_RPARE);
      node = new_node(ND_CAST, cast(), NULL);
      node->cast_to = type;
      return node;
    } else {
      return unary();
    }
  }
}

// unary       = postfix
//               |("++" | "--")+ unary
//               | unary_op cast
//               | sizeof unary
// unary_op    = ("+" | "-" | "&" | "*")?
Node* unary() {
  debug_put("unary\n");
  
  if (consume(TK_ADD)) {
    if (consume(TK_ADD)) {  // "++"
      Node *ident = unary();
      Node *node  = new_node(ND_ADD, new_node_number(1), ident);
      return new_node(ND_ASSIGN, ident, node);
    } else {
      return cast();
    }
  }
  if (consume(TK_SUB)) {
    if (consume(TK_SUB)) { // "--"
      Node *ident = unary();
      Node *node  = new_node(ND_SUB, ident, new_node_number(1));
      return new_node(ND_ASSIGN, ident, node);
    } else {
      return new_node(ND_SUB, new_node_number(0), cast());
    }
  }
  if (consume(TK_AND))
    return new_node(ND_ADDR, cast(), NULL);
  if (consume(TK_MUL))
    return new_node(ND_DEREF, cast(), NULL);

  if (consume(TK_SIZEOF)) {
    Node *node;
    if (consume(TK_LPARE)) {
      node = new_node(ND_SIZEOF, unary(), NULL);
      expect(TK_RPARE);
    } else {
      node = new_node(ND_SIZEOF, unary(), NULL);
    }
    return node;
  }

  return postfix();
}

// postfix     = primary
//               | primary ("++" | "--")
//               | primary "[" expr "]"
//               | primary "(" args_def ")"
//               | primary "." primary
//               | primary "->" primary
Node* postfix() {
  debug_put("postfix\n");
  Node *node = primary();
  
  if (look_token(TK_ADD, 0) && look_token(TK_ADD, 1)) { // ++
    consume(TK_ADD);
    consume(TK_ADD);
    node = new_node_post_inc(node, 1);
  } else if (look_token(TK_SUB, 0) && look_token(TK_SUB, 1)) { // --
    consume(TK_SUB);
    consume(TK_SUB);
    node = new_node_post_inc(node, -1);
  } else if (consume(TK_LPARE)) { // assuming function call
    node->func_name = node->name; // copy to func_name
    node->kind      = ND_FUNC_CALL;
    node->type      = new_type_int(); // TODO: supoprt any return type
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
    Node *add   = new_node(ND_ADD, node, expr());
    node = new_node(ND_DEREF, add, NULL);
    expect(TK_RBBRA);
  } else if (consume(TK_DOT)) {
    node = new_node(ND_DOT, node, expect_ident());
  } else if (consume(TK_ARROW)) {
    node = new_node(ND_ARROW, node, expect_ident());
  }

  return node;
}

// primary    = num | ident | "(" expr ")" | \"characters\"
Node* primary() {
  debug_put("primary\n");
  Node* node;
  char* name;
  // if '(' is found, 
  if (consume(TK_LPARE)) {
    node = expr();
    expect(TK_RPARE);   // ')' should be here
    return node;
  } else if (node = consume_literal()) { // literal
    return node;
  } else if (node = consume_ident()) {
    debug_print("%s found\n", node->name);
    get_ident_info(&node);
    debug_print("node->name:%s %d\n", node->name, node->var->type->size);
    return node;
  } else {
    return new_node_number(expect_number());
  }
}

// func_decl_def   = func_decl | func_def
// func_decl       = type ident ("(" args_def ")") ";"
// func_def        = type ident ("(" args_def ")") "{" expr "}"
// args_def        = ident_decl? ("," ident_decl)*
Node* func_decl_def(Node *func_def, Type *ret_type) {
  debug_put("fnc_def\n");
  Node *node;

  func_def->kind      = ND_FUNC_DEF;
  func_def->func_name = func_def->name;
  func_def->type      = ret_type;

  allocate_ident(&func_def);

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
        allocate_ident(&node);
        node->kind = ND_ARG_DECL;
        func_def->args_def[i_arg++] = node;
        if (consume(TK_RPARE))
          break;
        expect(TK_COMMA);
      }
      func_def->args_def[i_arg] = NULL;
      if (!look_token(TK_LCBRA, 0)) {
        // this is function declaration
        expect(TK_SEMICOLON);
        func_def->kind = ND_FUNC_DECL;
        env = env->parent;
        return func_def;
      }
      func_def->body = statement(); // must be compound statement
      env = env->parent;
      return func_def;
    }
  }

  return NULL;
}

// struct_members = ident_decl ";" (ident_decl";")* "}"
Map *struct_members() {
  Map *map = new_map();

  while (1) {
    Node *node = ident_decl();
    if (!node && consume(TK_RCBRA))
      break;
    expect(TK_SEMICOLON);
    map_add(map, node->name, node->type);
  }
  return map;
}