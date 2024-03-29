#include "9cc.h"

int lvar_offset;
Type *g_ret_type;

Node* do_walk(Node* node, bool decay);

bool is_32(Node *node) {
  if (node->type->ty == ARRAY)
    return false;
  return node->type->size == 4;
}
bool is_32_elem(Node *node) {
  if (node->type->ty == ARRAY)
    return node->type->ptr_to->size == 4;
  return node->type->size == 4;
}
bool is_8(Node *node) {
  if (node->type->ty == ARRAY)
    return false;
  return node->type->size == 1;
}
bool is_8_elem(Node *node) {
  if (node->type->ty == ARRAY)
    return node->type->ptr_to->size == 1;
  return node->type->size == 1;
}
bool is_int_type(Type *type) {
  return type->ty == INT || type->ty == ENUM;
}
bool is_int(Node *node) {
  return is_int_type(node->type);
}
void check_int(Node *node) {
  if (!is_int(node))
    error("got not int\n");
}
bool is_char_type(Type *type) {
  return type->ty == CHAR;
}
bool is_char(Node *node) {
  return is_char_type(node->type);
}
bool is_num_type(Type *type) {
  return is_int_type(type) || is_char_type(type);
}
bool is_num(Node *node) {
  return is_int(node) || is_char(node);
}
bool is_ptr_type(Type *type) {
  return type->ty == PTR || type->ty == ARRAY;
}
bool is_ptr(Node *node) {
  return is_ptr_type(node->type);
}
bool is_narrowing(Type *type, Type *type_to) {
  return type->size > type_to->size;
}
bool check_ptr(Node *node) {
  if (!is_ptr(node))
    error("got not ptr\n");
}
bool same_type(Type *t1, Type *t2) {
  if (t1->ty != t2->ty)
    return false;

  switch (t1->ty) {
  case PTR:
  case ARRAY:
    return same_type(t1->ptr_to, t2->ptr_to);
  default:
    return true;
  }
}
bool same_size(Node *n1, Node *n2) {
  return n1->type->size == n2->type->size;
}

Type *clone_type(Type *_type) {
  Type *type = _type;
  Type *clone = calloc(1, sizeof(Type));
  clone->ty = type->ty;
  clone->size = type->size;
  clone->align = type->align;

  if (!type->ptr_to)
    return clone;

  Type *ptr_to = clone_type(type->ptr_to);
  clone->ptr_to = ptr_to;
  return clone;
}
Node *default_return() {
  Node *ret = calloc(1, sizeof(Node));
  ret->kind = ND_RETURN;
  ret->lhs  = new_node_number(0);
}

bool valid_assignment(Node *node1, Node *node2) {
  return (is_num(node1) && is_num(node2)) || (is_ptr(node1) && is_ptr(node2));
}

Node* cast(Node* node, Type* to) {
  // preserve struct member offset before make clone
  int offset = node->type->offset;
  node->type = clone_type(to);
  node->type->offset = offset;
  return node;
}

Node *cast_to_same_type(Node *node) {
  int lsize = node->lhs->type->size;
  int rsize = node->rhs->type->size;
  if (lsize > rsize) {
    node->rhs = cast(node->rhs, node->lhs->type);
  } else if (lsize < rsize) {
    node->lhs = cast(node->lhs, node->rhs->type);
  }
  node->type = clone_type(node->lhs->type);
  return node;
}

Node* scale_ptr(NodeKind kind, Node *base, Type *type) {
  Node *node = new_node(kind, NULL, NULL);
  node->lhs  = base;
  node->rhs  = new_node_number(type->ptr_to->size);
  node->type = type;
  return node;
}

Node* walk(Node* node) {
  return do_walk(node, true);
}

Node* walk_nodecay(Node* node) {
  return do_walk(node, false);
}

Node* do_walk(Node* node, bool decay) {
  if (node == NULL)
    return NULL;
  debug_print("do_walk: %s %s\n", NODE_KIND_STR[node->kind], node->name);
  Type *member;
  int i_block = 0;

  switch (node->kind) {
  case ND_IDENT:
    debug_print("ident size %d %p\n", node->var->type->size, node->var);
    return node;
  case ND_NUM:
  case ND_LITERAL:
    return node;
  case ND_LVAR_INIT:
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    node = node->rhs; // rhs is assingment
    return node;
  case ND_LVAR_DECL:
    if (node->var->type->ty == VOID)
      error("local variable with void type");
    node->var->offset = lvar_offset + node->var->type->size;
    lvar_offset = lvar_offset + node->var->type->size;
    node->kind = ND_NONE;  // do nothing after semantic analysys
    debug_print("lvardecl size %d\n", node->var->type->size);
    return node;
  case ND_ARG_DECL:
    if (node->var->type->ty == ARRAY) {
      node->var->offset = lvar_offset + node->var->type->ptr_to->size;
    } else {
      node->var->offset = lvar_offset + node->var->type->size;
    }
    debug_print("arg_decl size %d\n", node->var->type->size);
    lvar_offset = lvar_offset + node->var->type->size;
    return node;
  case ND_ADD:
    debug_print("add start %s %d\n", node->lhs->name, node->rhs->val);
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);

    debug_print("add mid %p %p\n", node->lhs->type, node->rhs->type);
    if (is_ptr(node->lhs) && is_ptr(node->rhs)) {
      error("pointer addition is not supported");
    }
    if (is_num(node->lhs) && is_ptr(node->rhs)) {
      node->lhs = scale_ptr(ND_MUL, node->lhs, node->rhs->type);
      node->type = node->rhs->type;
    } else if (is_ptr(node->lhs) && is_num(node->rhs)) {
      node->rhs = scale_ptr(ND_MUL, node->rhs, node->lhs->type);
      node->type = node->lhs->type;
    } else {
      node = cast_to_same_type(node);
    }
    debug_print("add end %s %d\n", node->lhs->name, node->rhs->val);
    return node;
  case ND_SUB:
    debug_print("sub start %s %d\n", node->lhs->name, node->rhs->val);
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);

    debug_print("sub mid %p %p\n", node->lhs->type, node->rhs->type);
    if (is_ptr(node->lhs) && is_ptr(node->rhs)) {
      if (!same_type(node->lhs->type, node->rhs->type))
        error("pointer subtraction with different type");
      node       = scale_ptr(ND_DIV, node, node->lhs->type);
      node->type = new_type_int();
    } else {
      node = cast_to_same_type(node);
    }
    debug_print("sub end %d %d\n", node->lhs->type->ty, node->rhs->type->ty);
    debug_print("sub end %s %d\n", node->lhs->name, node->rhs->val);
    return node;
  case ND_MUL:
  case ND_DIV:
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    if(same_type(node->lhs->type, node->rhs->type)) {
      node->type = node->lhs->type;
    } else {
      error("mult or div with different type\n");
    }
    return node;
  case ND_CAST:
    node->lhs = walk(node->lhs);
    if (is_narrowing(node->lhs->type, node->cast_to)) {
      fprintf(stderr, "Warning : narrowing conversion %s in line %d\n",
              node->lhs->name, get_line_number(node->lhs->token));
    }
    node = cast(node->lhs, node->cast_to);
    return node;
  case ND_EQ:
  case ND_NE:
  case ND_GT:
  case ND_GE:
  case ND_LT:
  case ND_LE:
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    if(same_type(node->lhs->type, node->rhs->type)) {
      node->type = node->lhs->type;
    } else {
      error("comparsion with different type\n");
    }
    return node;
  case ND_POSTINC:
    node->rhs = walk(node->rhs);
    node->statement = walk(node->statement);
    node->type = node->rhs->type;
    return node;
  case ND_DOT:
    node->lhs = walk(node->lhs);
    if (node->lhs->type->ty != STRUCT)
      error("invalid dot operation");
    member = map_find(node->lhs->type->members, node->rhs->name);
    if (!member)
      error("%s not found in member list of %s", node->rhs->name, node->lhs->name);
    debug_print("member ****** %s %d\n", node->rhs->name, member->offset);
    node->type = member;
    return node;
  case ND_ARROW:
    node->lhs = walk(node->lhs);
    if (node->lhs->type->ty != PTR)
      error("invalid arrow operation on not PTR");
    if (node->lhs->type->ptr_to->ty != STRUCT)
      error("invalid arrow operation on PTR of not STRUCT");
    member = map_find(node->lhs->type->ptr_to->members, node->rhs->name);
    if (!member)
      error("%s not found in member list of %s", node->rhs->name, node->lhs->type->ptr_to->name);
    debug_print("member ****** %s %d\n", node->rhs->name, member->offset);
    node->type = member;
    return node;
  case ND_SIZEOF:
    node->lhs  = walk(node->lhs);
    if (node->lhs->type->ty == ARRAY) {
      node->val = node->lhs->type->array_size;
    } else {
      node->val = node->lhs->type->size;
    }
    node->kind = ND_NUM;
    node->type = new_type_int();
    return node;
  case ND_RETURN:
    debug_put("return start\n");
    node->lhs = walk(node->lhs);
    node->type = node->lhs->type;
    if (is_num(node->lhs) && is_ptr_type(g_ret_type))
      warning("return makes pointer from integer without a cast");
    if (is_ptr(node->lhs) && is_num_type(g_ret_type))
      warning("return makes integer from pointer without a cast");
    debug_put("return end\n");
    return node;
  case ND_DEREF:
    debug_put("deref start\n");
    node->lhs = walk(node->lhs);
    check_ptr(node->lhs);
    node->type = node->lhs->type->ptr_to;
    debug_put("deref end\n");
    return node;
  case ND_ADDR:
    debug_put("addr start\n");
    node->lhs = walk(node->lhs);
    Type *type = new_type(PTR, node->lhs->type);
    node->type = type;
    debug_put("addr end\n");
    return node;
  case ND_ASSIGN:
    debug_print("assign start %s %s\n", node->lhs->name, node->rhs->name);
    node->lhs = walk(node->lhs);
    debug_put("assign mid\n");
    node->rhs = walk(node->rhs);
    if (node->lhs->type->is_const)
      error("assignment of read-only variable %s", node->lhs->var->name);
    if (node->rhs->type->is_const)
      warning("assignment discards \'const\' qualifier");
    if (!valid_assignment(node->lhs, node->rhs)) {
      error("invalid assignment");
    }
    if (is_8(node->lhs)) {
      node->rhs = cast(node->rhs, new_type_char());
    }
    node->type = node->lhs->type;
    debug_put("assign end:\n");
    return node;
  case ND_VAR_INIT:
    debug_print("assign start %s %s\n", node->lhs->name, node->rhs->name);
    node->lhs = walk(node->lhs);
    debug_put("assign mid\n");
    node->rhs = walk(node->rhs);
    if (!valid_assignment(node->lhs, node->rhs)) {
      error("invalid assignment");
    }
    if (is_8(node->lhs)) {
      node->rhs = cast(node->rhs, new_type_char());
    }
    node->type = node->lhs->type;
    debug_put("assign end:\n");
    return node;
  case ND_FOR:
    node->init      = walk(node->init);
    node->condition = walk(node->condition);
    node->last      = walk(node->last);
    node->statement = walk(node->statement);
    return node;
  case ND_WHILE:
    node->condition = walk(node->condition);
    node->statement = walk(node->statement);
    return node;
  case ND_IF:
    node->condition      = walk(node->condition);
    node->if_statement   = walk(node->if_statement);
    node->else_statement = walk(node->else_statement);
    return node;
  case ND_BLOCK:
    debug_put("block start\n");
    Vector *new_block = new_vector();
    // eliminate ND_NONE node from block
    for (int i = 0; i < node->block->len; i++) {
      Node *elem = walk(vector_get(node->block, i));
      if (elem->kind != ND_NONE)
        vector_push_back(new_block, elem);
    }
    node->block = new_block;
    debug_put("block end\n");
    return node;
  case ND_FUNC_CALL:
    while (node->args_call[i_block]) {
      node->args_call[i_block] = walk(node->args_call[i_block]);
      i_block++;
    }
    return node;
  default:
    error("unsupported\n");
    return node;
  }
}

Vector *sema(Vector *prog) {
  debug_print("prog lenght %d\n", prog->len);
  Vector *new_vec = new_vector();
  for (int i = 0; i < prog->len; i++) {
    debug_print("get %d\n", i);
    Node *_prog = vector_get(prog, i);
    if (_prog->kind == ND_GVAR_DECL) {
      vector_push_back(new_vec, _prog);
    } else if (_prog->kind == ND_FUNC_DECL) {
      _prog->kind = ND_NONE;
    } else if (_prog->kind == ND_FUNC_DEF) {
      int i_arg = 0;
      lvar_offset = 0;
      g_ret_type = _prog->type;
      while (_prog->args_def[i_arg]) {
        _prog->args_def[i_arg] = walk(_prog->args_def[i_arg]);
        i_arg++;
      }
      walk(_prog->body);
      // make sure return is called
      // FIXME: should determine whether this is required or not for cleaner assembly
      vector_push_back(_prog->body->block, default_return());
      _prog->locals_size = lvar_offset;
      vector_push_back(new_vec, _prog);
    } else {
      error("unexpected toplevel node %s\n", NODE_KIND_STR[_prog->kind]);
    }
  }
  return new_vec;
}