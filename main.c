#include <errno.h>
#include "9cc.h"

Token* token;
Vector* vec_token;

char* read_file(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp)
    error("%s : fopen failed %s", filename, strerror(errno));

  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s : fseek failed %s", filename, strerror(errno));
  size_t len = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s : fseek failed %s", filename, strerror(errno));

  char *buf = calloc(1, len + 2);
  fread(buf, len, 1, fp);
  debug_print("%ld\n", len) ;

  if (len == 0 || buf[len - 1] != '\n')
    buf[len++] = '\n';
  buf[len] = '\0';
  fclose(fp);
  return buf;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  // char* p = read_file(argv[1]);
  char* p = argv[1];
  token = tokenize(p);
  debug_put("tokenize is done\n");
  // print_token_recursive(token);

  vec_token = preprocess();
  debug_put("preprocess is done\n");

  program();
  // Node* node = expr();
  debug_put("program is read\n");

  sema();

  int i = 0;
  while (prog[i]) {
    debug_print("program processing %d\n", i);
    gen(prog[i]);
    debug_print("program processed %d\n", i);
    i++;
  }

  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  debug_put("compile done.\n");
  return 0;
}