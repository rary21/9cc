#include "9cc.h"

Token* token;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  char* p = argv[1];
  token = tokenize(p);
  // print_token_recursive(token);

  program();
  // Node* node = expr();
  debug_put("program is read\n");

  sema();

  int i = 0;
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  while (prog[i]) {
    debug_print("program processing %d\n", i);
    gen(prog[i++]);
  }

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}