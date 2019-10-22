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
1,        // left braces  "["
1,        // right braces "]"
1,        // single quotation
1,        // double quotation
1,        // literal
1,        // "&"
1,        // "#"
1,        // "\\"
1,        // "\n"
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
1,        // dot "."
2,        // arrow "->"
6,        // return
2,        // if
4,        // else
5,        // while
3,        // for
4,        // void
3,        // int
4,        // char
6,        // struct
6,        // define
6,        // sizeof
7,        // typedef
6,        // static
5,        // const
1,        // End Of File
};

const char* TOKEN_KIND_STR[NUM_TOKEN_KIND] =
  {"TK_ADD", "TK_SUB", "TK_MUL", "TK_DIV", "TK_LPARE", "TK_RPARE", "TK_LCBRA", "TK_RCBRA", "TK_LBBRA",
   "TK_RBBRA", "TK_SQUOT", "TK_DQUOT", "TK_LITERAL", "TK_AND", "TK_SHARP", "TK_BACKSLASH", "TK_NEWLINE", "TK_NUM", "TK_EQ", "TK_NE",
   "TK_LT", "TK_LE", "TK_GT", "TK_GE", "TK_IDENT", "TK_ASSIGN", "TK_SEMICOLON", "TK_COMMA", "TK_DOT", "TK_ARROW", "TK_RETURN",
   "TK_IF", "TK_ELSE", "TK_WHILE", "TK_FOR", "TK_VOID", "TK_INT", "TK_CHAR", "TK_STRUCT", "TK_DEFINE", "TK_SIZEOF", "TK_TYPEDEF", 
   "TK_STATIC", "TK_CONST", "TK_EOF"};

typedef struct Env Env;
struct Env {
  char *buf;
};

Env *env;

static Env* new_env(char* buf) {
  Env *env = calloc(1, sizeof(Env));
  env->buf = buf;
  return env;
}

// return true if c is expected to skip
bool isskip(const char c) {
  // don't skip newline
  if (c == '\n')
    return false;
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
bool is_alpha(const char c) {
  return isalnum(c) || c == '_';
}

int get_ident_len(char* str) {
  int i = 0;
  // ident must starts with alphabet or underscore
  if (!is_alpha(str[0]))
    return 0;

  for (i = 1; str[i]; i++) {
    // alphabet, number or underscore
    if (!is_alnum(str[i]))
      break;
  }
  return i;
}

int get_line_number(Token *tkn) {
  int line_cnt = 1;
  for (char *p = tkn->buf; p != tkn->end; p++) {
    if (*p == '\n')
      line_cnt++;
  }
  return line_cnt;
}

// append new token to cur and return pointer to new token
Token* new_token(Token *cur, TokenKind kind, char **str) {
  Token *next = (Token*)calloc(1, sizeof(Token));
  int len;
  if (kind == TK_IDENT)
    len = get_ident_len(*str);
  else
    len = TK_LEN_OF_KIND[kind];
  cur->next   = next;
  next->kind  = kind;
  next->len   = len;
  next->start = *str;
  next->buf   = env->buf;

  char *tmp = calloc(1, len + 1);
  strncpy(tmp, *str, len);
  tmp[len] = '\0';
  next->str   = tmp;

  char s[256];
  strncpy(s, *str, len);
  s[len] = '\0';
  debug_print("** new_token : %s %s\n", TOKEN_KIND_STR[kind], s);

  if (kind == TK_NUM)
    next->val  = strtol(*str, str, 10);
  else
    *str = *str + len;
  next->end   = *str;

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
  if (strncmp(p, "->", 2) == 0) {
    *kind = TK_ARROW;
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
  if (strncmp(p, "void", 4) == 0 && !is_alnum(*(p+4))) {
    *kind = TK_VOID;
    return true;
  }
  if (strncmp(p, "int", 3) == 0 && !is_alnum(*(p+3))) {
    *kind = TK_INT;
    return true;
  }
  if (strncmp(p, "char", 4) == 0 && !is_alnum(*(p+4))) {
    *kind = TK_CHAR;
    return true;
  }
  if (strncmp(p, "struct", 6) == 0 && !is_alnum(*(p+6))) {
    *kind = TK_STRUCT;
    return true;
  }
  if (strncmp(p, "define", 6) == 0 && !is_alnum(*(p+6))) {
    *kind = TK_DEFINE;
    return true;
  }
  if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(*(p+6))) {
    *kind = TK_SIZEOF;
    return true;
  }
  if (strncmp(p, "typedef", 7) == 0 && !is_alnum(*(p+7))) {
    *kind = TK_TYPEDEF;
    return true;
  }
  if (strncmp(p, "static", 6) == 0 && !is_alnum(*(p+6))) {
    *kind = TK_STATIC;
    return true;
  }
  if (strncmp(p, "const", 5) == 0 && !is_alnum(*(p+5))) {
    *kind = TK_CONST;
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
  if (*p == '#') {
    *kind = TK_SHARP;
    return true;
  }
  if (*p == '\\') {
    *kind = TK_BACKSLASH;
    return true;
  }
  if (*p == '\n') {
    *kind = TK_NEWLINE;
    return true;
  }
  if (*p == '(') {
    *kind = TK_LPARE;
    return true;
  }
  if (*p == ')') {
    *kind = TK_RPARE;
    return true;
  }
  if (*p == '{') {
    *kind = TK_LCBRA;
    return true;
  }
  if (*p == '}') {
    *kind = TK_RCBRA;
    return true;
  }
  if (*p == '[') {
    *kind = TK_LBBRA;
    return true;
  }
  if (*p == ']') {
    *kind = TK_RBBRA;
    return true;
  }
  if (*p == '\'') {
    *kind = TK_SQUOT;
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
  if (*p == '.') {
    *kind = TK_DOT;
    return true;
  }
  if (is_alnum(*p)) {
    *kind = TK_IDENT;
    return true;
  }
  return false;
}

void copy_and_replace_backslash(char *dst, char *src) {
  int newline_cnt = 0;
  while (*src) {
    if (*(src+1) && strncmp(src, "\\\n", 2) == 0) {
      src += 2;
      newline_cnt++;
      continue;
    }
    if (*src == '\n') {
      for (int i = 0; i < newline_cnt; i++) {
        *dst = '\n';
      }
    }
    *dst++ = *src++;
  }
  *dst = '\0';
}

Token* tokenize(char *q) {
  TokenKind kind;
  Token head;
  Token *cur = &head;
  char *p;
  int len;
  int literal_id = 0;

  len = strlen(q);
  p = calloc(1, len + 1);

  copy_and_replace_backslash(p, q);

  env = new_env(p);

  while (*p) {
    // skip some characters;
    if (isskip(*p)) {
      p++;
      continue;
    }
    if (0 == strncmp(p, "//", 2)) {
      while (*p && *p++ != '\n');
      continue;
    }
    if (0 == strncmp(p, "/*", 2)) {
      while (*p && strncmp(p++, "*/", 2));
      p++;
      continue;
    }
    if (*p == '\"') {
      p++;
      Token *next = (Token*)calloc(1, sizeof(Token));
      cur->next   = next;
      next->kind  = TK_LITERAL;
      next->start = p;
      next->buf   = env->buf;
      printf(".LC%d:\n", literal_id);
      next->literal_id = literal_id++;
      cur->next  = next;
      printf("  .string \"");
      while (*p && *p != '\"') {
        putchar(*p);
        p++;
      }
      printf("\"\n");
      next->end = p;
      next->len = next->end - next->start;

      char *str = calloc(1, next->len + 1);
      strncpy(str, next->start, next->len);
      str[next->len] = '\0';
      next->str   = str;

      p++;
      cur->next  = next;
      cur = next;
      debug_print("** new_token : %s\n", TOKEN_KIND_STR[cur->kind]);
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