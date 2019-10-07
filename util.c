#include "9cc.h"

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}


void print_token(Token *tkn) {
  debug_print("kind: %d value: %d str[0]: %c\n", tkn->kind, tkn->val, tkn->str[0]);
}

void print_token_recursive(Token *tkn) {
  while (tkn->kind != TK_EOF) {
    print_token(tkn);
    tkn = tkn->next;
  }
}

void print_node(Node *node) {
  debug_print("kind: %s value: %d\n", NODE_KIND_STR[node->kind], node->val);
}

void print_node_recursive(Node *node) {
  if (node->lhs != NULL) {
    print_node_recursive(node->lhs);
  }
  print_node(node);
  if (node->rhs != NULL) {
    print_node_recursive(node->rhs);
  }
}