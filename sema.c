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

Node* scale_ptr(NodeKind kind, Node *base, Type *type) {
  Node *node = new_node(kind, NULL, NULL);
  node->lhs = base;
  node->rhs = new_node_number(type->ptr_to->size);
  return node;
}

Node* walk(Node* node) {
  return do_walk(node, true);
}

Node* walk_nodecay(Node* node) {
  return do_walk(node, false);
}

Node* do_walk(Node* node, bool decay) {
  fprintf(stderr, "do_walk: %s\n", NODE_KIND_STR[node->kind]);
  int i_block = 0;

  switch (node->kind) {
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
  case ND_RETURN:
    fprintf(stderr, "return start\n");
    node->lhs = walk(node->lhs);
    fprintf(stderr, "return end\n");
    return node;
  case ND_DEREF:
    fprintf(stderr, "deref start\n");
    node->lhs = walk(node->lhs);
    check_ptr(node->lhs);
    node->type = node->lhs->type->ptr_to;
    fprintf(stderr, "deref end\n");
    return node;
  case ND_ASSIGN:
    node->lhs = walk(node->lhs);
    node->rhs = walk(node->rhs);
    return node;
  case ND_BLOCK:
    while (node->block[i_block]) {
      walk(node->block[i_block++]);
    }
    return node;
  default:
    return node;
  }
}

void sema() {
  int i_prog = 0;
  while(prog[i_prog]) {
    walk(prog[i_prog++]->body);
  }
}