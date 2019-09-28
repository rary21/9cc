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

  program();
  // Node* node = expr();

  int i = 0;
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");
  while (prog[i]) {
    print_node_recursive(prog[i]);
    gen(prog[i++]);
    printf("  pop rax   # in loop\n");
  }

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}