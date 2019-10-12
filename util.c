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

Vector *new_vector() {
  // initial vector size is 8
  int len = 8;
  Vector *vec   = calloc(1, sizeof(Vector));
  vec->elem     = calloc(1, len * sizeof(void *));
  vec->capacity = len;
  vec->len      = 0;

  return vec;
}

void vector_push(Vector *vec, void *p) {
  // expand vector size
  if (vec->len == vec->capacity) {
    vec->elem     = realloc(vec->elem, 2 * vec->capacity * sizeof(void *));
    vec->capacity = 2 * vec->capacity;
  }

  vec->elem[vec->len] = p;
  vec->len++;
}

void *vector_get(Vector *vec, int i) {
  return vec->elem[i];
}