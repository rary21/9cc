#include "9cc.h"

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
  if (node->type->ty != INT)
    error("got not int\n");
}
bool is_ptr(Node *node) {
  return node->type->ty == PTR;
}
bool check_ptr(Node *node) {
  if (node->type->ty != PTR)
    error("got not ptr\n");
}
bool same_type(Node *n1, Node *n2) {
  if (is_int(n1) && is_int(n2))
    return true;
  if (is_int(n1) || is_int(n2))
    return false;
  if (n1->type->nptr == n1->type->nptr)
    return true;
  return false;
}
bool same_size(Node *n1, Node *n2) {
  return n1->type->size == n2->type->size;
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
  fprintf(stderr, "do_walk: %s %s\n", NODE_KIND_STR[node->kind], node->name);
  int i_block = 0;

  switch (node->kind) {
  case ND_IDENT:
  case ND_NUM:
    return node;
  case ND_ADD:
    fprintf(stderr, "add start %s %d\n", node->lhs->name, node->rhs->val);
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);

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
    fprintf(stderr, "add end %s %d\n", node->lhs->name, node->rhs->val);
    return node;
  case ND_SUB:
    return node;
  case ND_MUL:
  case ND_DIV:
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    if(same_type(node->lhs, node->rhs)) {
      node->type = node->lhs->type;
    } else {
      error("mult or div with different type\n");
    }
    return node;
  case ND_GT:
  case ND_GE:
  case ND_LT:
  case ND_LE:
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    if(same_type(node->lhs, node->rhs)) {
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
    fprintf(stderr, "return start\n");
    node->lhs = walk(node->lhs);
    node->type = node->lhs->type;
    fprintf(stderr, "return end\n");
    return node;
  case ND_DEREF:
    fprintf(stderr, "deref start\n");
    node->lhs = walk(node->lhs);
    check_ptr(node->lhs);
    node->type = node->lhs->type->ptr_to;
    fprintf(stderr, "deref end\n");
    return node;
  case ND_ADDR:
    fprintf(stderr, "addr start\n");
    node->lhs = walk(node->lhs);
    Type *type = new_type(PTR, node->lhs->type);
    node->type = type;
    fprintf(stderr, "addr end\n");
    return node;
  case ND_ASSIGN:
    fprintf(stderr, "assign start %s %s\n", node->lhs->name, node->rhs->name);
    node->lhs = walk(node->lhs);
    fprintf(stderr, "assign mid\n");
    node->rhs = walk(node->rhs);
    node->type = node->lhs->type;
    fprintf(stderr, "assign end:\n");
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
    while (node->block[i_block]) {
      fprintf(stderr, "i_block %d\n", i_block);
      node->block[i_block] = walk(node->block[i_block]);
      i_block++;
    }
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
    walk(prog[i_prog]->body);
    prog[i_prog]->locals_size = get_lvars_size(prog[i_prog]->locals);
    i_prog++;
  }
}