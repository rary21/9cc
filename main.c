#include "9cc.h"

Token* token;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  char* p = argv[1];
  token = tokenize(p);
  // print_token_recursive(token);

  Node* node = expr();
  // print_node_recursive(node);

  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}