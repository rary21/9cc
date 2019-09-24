#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#define debug_print(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
#define debug_print(fmt, ...)
#endif

// kind of token
typedef enum {
  TK_OP,          // operations
  TK_NUM,         // digits
  TK_EOF,         // End Of File
} TokenKind;

typedef struct Token Token;

// Token
struct Token {
  TokenKind kind; // kind of token
  Token *next;    // pointer to next token
  long int val;        // value when kind is TK_NUM
  char *str;      // string of token
};

// current token
Token *token;

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// go to next token and return true if current token is TK_OP and matches with op
// otherwise, return false
bool consume(const char op) {
  if (token->kind != TK_OP || token->str[0] != op)
    return false;
  // next token
  debug_print("op: %c  token->kind: %d token->str[0]: %c\n", op, token->kind, token->str[0]);
  token = token->next;
  return true;
}

// go to next token and return true if current token is TK_OP and matches with op
// otherwise, throw error and exit application
void expect(const char op) {
  if (token->kind != TK_OP || token->str[0] != op)
    error("expect error");
  // next token
  token = token->next;
}

// go to next token and return value if current token is TK_NUM
// otherwise, throw error and exit application
int consume_number() {
  if (token->kind != TK_NUM)
    error("expect error");
  // next token
  int value = token->val;
  token = token->next;
  return value;
}

// return true if c is expected to skip
bool isskip(const char c) {
  if (c == ' ')
    return true;
  return false;
}

// return true if c is operation
bool isoperation(const char c) {
  if (c == '+')
    return true;
  if (c == '-')
    return true;
  return false;
}

void print_token(Token *tkn) {
  debug_print("kind: %d value: %ld str[0]: %c\n", tkn->kind, tkn->val, tkn->str[0]);
}

void print_token_recursive(Token *tkn) {
  while (tkn->kind != TK_EOF) {
    print_token(tkn);
    tkn = tkn->next;
  }
}

// append new token to cur and return pointer to new token
Token* new_token(Token *cur, TokenKind kind, char *str) {
  Token *next = (Token*)calloc(1, sizeof(Token));
  cur->next = next;
  next->kind = kind;
  next->str  = str;
  return next;
}

Token* tokenize(char *p) {
  Token head;
  Token *cur = &head;

  while (*p) {
    // skip some characters;
    if (isskip(*p)) {
      p++;
      continue;
    }
    if (isoperation(*p)) {
      cur = new_token(cur, TK_OP, p);
      p++;
      continue;
    }
    if (isdigit(*p)) {
      cur = new_token(cur, TK_NUM, p);
      cur->val  = strtol(p, &p, 10);
      continue;
    }
    error("cannot tokenize");
  }

  cur = new_token(cur, TK_EOF, p);

  return head.next;
}

// return true if current token is eof
bool is_eof(Token* tkn) {
  return tkn->kind == TK_EOF;
}

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
  print_token_recursive(token);
  long int val = consume_number();
  printf("  mov rax, %ld\n", val);
  while (token->kind != TK_EOF) {
    if (consume('+')) {
      val = consume_number();
      printf("  add rax, %ld\n", val);
      continue;
    }
    if (consume('-')) {
      val = consume_number();
      printf("  sub rax, %ld\n", val);
      continue;
    }
    if (is_eof(token))
      break;
    error("error in main loop");
  }

  printf("  ret\n");
  return 0;
}