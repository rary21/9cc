#include "9cc.h"

int lvar_offset = 0;

Node* do_walk(Node* node, bool decay);

bool is_32(Node *node) {
  return node->type->size == 4;
}
bool is_8(Node *node) {
  return node->type->size == 1;
}
bool is_int(Node *node) {
  return node->type->ty == INT;
}
void check_int(Node *node) {
  if (!is_int(node))
    error("got not int\n");
}
bool is_ptr(Node *node) {
  return node->type->ty == PTR || node->type->ty == ARRAY;
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

Node* cast(Node* node, Type* to) {
  node->type = to;
  return node;
}

Node* scale_ptr(NodeKind kind, Node *base, Type *type) {
  Node *node = new_node(kind, NULL, NULL);
  node->lhs  = base;
  node->rhs  = new_node_number(type->ptr_to->size);
  node->type = type;
  return node;
}

// compute total size of local variables
int get_lvars_size(LVar *lvars) {
  LVar* lvar;
  int size = 0;
  for (lvar = lvars; lvar; lvar = lvar->next) {
    size = size + lvar->type->size;
  }
  return size;
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
  int i_block = 0;

  switch (node->kind) {
  case ND_IDENT:
    debug_print("ident size %d %p\n", node->var->type->size, node->var);
    return node;
  case ND_NUM:
  case ND_LITERAL:
    return node;
  case ND_LVAR_DECL:
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
    if (is_int(node->lhs) && is_ptr(node->rhs)) {
      node->lhs = scale_ptr(ND_MUL, node->lhs, node->rhs->type);
      node->type = node->rhs->type;
    } else if (is_ptr(node->lhs) && is_int(node->rhs)) {
      node->rhs = scale_ptr(ND_MUL, node->rhs, node->lhs->type);
      node->type = node->lhs->type;
    } else {
      node->type = new_type_int();
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
      node->type = node->lhs->type;
    } else {
      node->type = new_type_int();
    }
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
    while (node->block[i_block]) {
      debug_print("i_block %d\n", i_block);
      node->block[i_block] = walk(node->block[i_block]);
      i_block++;
    }
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

void sema() {
  int i_prog = 0;
  while(prog[i_prog]) {
    if (prog[i_prog]->kind == ND_GVAR_DECL) {
      i_prog++;
    } else if (prog[i_prog]->kind == ND_FUNC_DEF) {
      int i_arg = 0;
      debug_print("sema %d %p\n", i_prog, prog[i_prog]->body);
      lvar_offset = 0;
      while (prog[i_prog]->args_def[i_arg]) {
        prog[i_prog]->args_def[i_arg] = walk(prog[i_prog]->args_def[i_arg]);
        i_arg++;
      }
      walk(prog[i_prog]->body);
      prog[i_prog]->locals_size = lvar_offset;
      debug_print("sema %d offset%d\n", i_prog, prog[i_prog]->locals_size);
      i_prog++;
    } else {
      error("unexpected toplevel node %s\n", NODE_KIND_STR[prog[i_prog]->kind]);
    }
  }
}