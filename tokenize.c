#include "9cc.h"

unsigned int TK_LEN_OF_KIND[NUM_TOKEN_KIND] = {
1,        // addition
1,        // subtraction
1,        // multiplication
1,        // division
1,        // left parenthesis "("
1,        // right parenthesis ")"
1,        // left braces  "{"
1,        // right braces "}"
1,        // "&"
1,        // digits (must be 1 since this will be used in strncmp)
2,        // "=="
2,        // "!="
1,        // "<
2,        // "<=
1,        // ">
2,        // ">=
1,        // variable
1,        // assignment "="
1,        // semicolon ";"
1,        // comma ","
6,        // return
2,        // if
4,        // else
5,        // while
3,        // for
3,        // int
1,        // End Of File
};

const char* TOKEN_KIND_STR[NUM_TOKEN_KIND] =
  {"TK_ADD", "TK_SUB", "TK_MUL", "TK_DIV", "TK_LPAR", "TK_RPAR", "TK_LBRA", "TK_RBRA", "TK_AND",
   "TK_NUM", "TK_EQ", "TK_NE", "TK_LT", "TK_LE", "TK_GT", "TK_GE", "TK_IDENT", "TK_ASSIGN",
   "TK_SEMICOLON", "TK_COMMA", "TK_RETURN", "TK_IF", "TK_ELSE", "TK_WHILE", "TK_FOR", "TK_INT", "TK_EOF"};

// return true if c is expected to skip
bool isskip(const char c) {
  if (isspace(c))
    return true;
  return false;
}

// return true if c is operation
bool isoperation(const char c) {
  if (c == '+')
    return true;
  if (c == '-')
    return true;
  if (c == '*')
    return true;
  if (c == '/')
    return true;
  return false;
}

bool is_alnum(const char c) {
  return isalnum(c) || c == '_';
}

int get_ident_len(char* str) {
  int i = 0;
  // ident must starts with alphabet
  if (!isalpha(str[0]))
    return 0;

  for (i = 1; str[i]; i++) {
    // alphabet, number or underscore
    if (!is_alnum(str[i]))
      break;
  }
  return i;
}

// append new token to cur and return pointer to new token
Token* new_token(Token *cur, TokenKind kind, char **str) {
  debug_print("** new_token : %s\n", TOKEN_KIND_STR[kind]);
  Token *next = (Token*)calloc(1, sizeof(Token));
  int len;
  if (kind == TK_IDENT)
    len = get_ident_len(*str);
  else
    len = TK_LEN_OF_KIND[kind];
  cur->next  = next;
  next->kind = kind;
  next->str  = *str;
  next->len  = len;
  if (kind == TK_NUM)
    next->val  = strtol(*str, str, 10);
  else
    *str = *str + len;
  return next;
}

bool get_kind(char *p, TokenKind *kind) {
  if (strncmp(p, "==", 2) == 0) {
    *kind = TK_EQ;
    return true;
  }
  if (strncmp(p, "!=", 2) == 0) {
    *kind = TK_NE;
    return true;
  }
  if (strncmp(p, "<=", 2) == 0) {
    *kind = TK_LE;
    return true;
  }
  if (strncmp(p, ">=", 2) == 0) {
    *kind = TK_GE;
    return true;
  }
  if (strncmp(p, "return", 6) == 0 && !is_alnum(*(p+6))) {
    *kind = TK_RETURN;
    return true;
  }
  if (strncmp(p, "if", 2) == 0 && !is_alnum(*(p+2))) {
    *kind = TK_IF;
    return true;
  }
  if (strncmp(p, "else", 4) == 0 && !is_alnum(*(p+4))) {
    *kind = TK_ELSE;
    return true;
  }
  if (strncmp(p, "while", 5) == 0 && !is_alnum(*(p+5))) {
    *kind = TK_WHILE;
    return true;
  }
  if (strncmp(p, "for", 3) == 0 && !is_alnum(*(p+3))) {
    *kind = TK_FOR;
    return true;
  }
  if (strncmp(p, "int", 3) == 0 && !is_alnum(*(p+3))) {
    *kind = TK_INT;
    return true;
  }
  if (*p == '+') {
    *kind = TK_ADD;
    return true;
  }
  if (*p == '-') {
    *kind = TK_SUB;
    return true;
  }
  if (*p == '*') {
    *kind = TK_MUL;
    return true;
  }
  if (*p == '/') {
    *kind = TK_DIV;
    return true;
  }
  if (*p == '&') {
    *kind = TK_AND;
    return true;
  }
  if (*p == '(') {
    *kind = TK_LPAR;
    return true;
  }
  if (*p == ')') {
    *kind = TK_RPAR;
    return true;
  }
  if (*p == '{') {
    *kind = TK_LBRA;
    return true;
  }
  if (*p == '}') {
    *kind = TK_RBRA;
    return true;
  }
  if (isdigit(*p)) {
    *kind = TK_NUM;
    return true;
  }
  if (*p == '<') {
    *kind = TK_LT;
    return true;
  }
  if (*p == '>') {
    *kind = TK_GT;
    return true;
  }
  if (*p == '=') {
    *kind = TK_ASSIGN;
    return true;
  }
  if (*p == ';') {
    *kind = TK_SEMICOLON;
    return true;
  }
  if (*p == ',') {
    *kind = TK_COMMA;
    return true;
  }
  if (isalpha(*p)) {
    *kind = TK_IDENT;
    return true;
  }
  return false;
}

Token* tokenize(char *p) {
  TokenKind kind;
  Token head;
  Token *cur = &head;

  while (*p) {
    // skip some characters;
    if (isskip(*p)) {
      p++;
      continue;
    }
    if (get_kind(p, &kind)) {
      cur = new_token(cur, kind, &p);
      continue;
    }
    error("cannot tokenize");
  }

  cur = new_token(cur, TK_EOF, &p);

  return head.next;
}