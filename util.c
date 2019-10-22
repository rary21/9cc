#include "9cc.h"

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void warning(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

void error_node(Node *node, char *fmt, ...) {
  Token *token;
  while (1) {
    node = node->lhs;
    if (node->token)
      break;
    node = node->rhs;
    if (node->token)
      break;
    node = node->lhs;
  }
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "LINE:%d ", get_line_number(token));
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

void vector_push_back(Vector *vec, void *p) {
  // expand vector size
  if (vec->len == vec->capacity) {
    vec->elem     = realloc(vec->elem, 2 * vec->capacity * sizeof(void *));
    vec->capacity = 2 * vec->capacity;
  }

  vec->elem[vec->len] = p;
  vec->len++;
}

void *vector_pop_front(Vector *vec) {
  if (vec->index > vec->len)
    return NULL;
  return vec->elem[vec->index++];
}

void *vector_get(Vector *vec, int i) {
  return vec->elem[i];
}

void *vector_get_front(Vector *vec) {
  return vec->elem[vec->index];
}

Map *new_map() {
  Map *map = calloc(1, sizeof(Map));
  map->keys = new_vector();
  map->vals = new_vector();
}

void map_add(Map *map, char *key, void *val) {
  for (int i = 0; i < map->keys->len; i++) {
    char *_key = vector_get(map->keys, i);
    if (strlen(key) != strlen(_key))
      continue;
    if (0 == strncmp(key, _key, strlen(key)))
      error("%s is already definied", key);
  }
  vector_push_back(map->keys, key);
  vector_push_back(map->vals, val);
  map->len++;
}

// NULL terminated key should be given
void *map_find(Map *map, const char* key) {
  for (int i = 0; i < map->keys->len; i++) {
    if (strlen(key) != strlen(map->keys->elem[i]))
      continue;
    if (0 == strncmp(map->keys->elem[i], key, strlen(key)))
      return map->vals->elem[i];
  }
  return NULL;
}

int roundup(int val, int align) {
  int rem = val % align;
  return (rem == 0) ? val : val + align - rem;
}