#include "9cc.h"

typedef struct Macro Macro;
struct Macro {
  Token *identifier;
  Vector *params;
  Vector *replacement;
  bool with_param;
};

typedef struct Env Env;
struct Env {
  Vector *macros;
};

Env *env;
Vector *g_vec;

Env* new_env() {
  Env *env = calloc(1, sizeof(Env));
  env->macros = new_vector();
  return env;
}

static bool consume(TokenKind kind) {
  if (token->kind != kind)
    return false;
  // next token
  debug_print("** consuming token->kind: %s\n",
               TOKEN_KIND_STR[token->kind]);
  token = token->next;
  return true;
}

void emit(void *p) {
  vector_push_back(g_vec, p);
}

// go to next token and return true if current token has same kind
// otherwise, throw error and exit application
static void expect(TokenKind kind) {
  if (token->kind != kind)
    error("%s expected, but got %s", TOKEN_KIND_STR[kind], TOKEN_KIND_STR[token->kind]);
  // next token
  token = token->next;
}

Vector *get_tokens_until_newline() {
  Vector *vec = new_vector();
  while (token->kind != TK_NEWLINE) {
    vector_push_back(vec, token);
    token = token->next;
  }
  return vec;
}

void generate_define_macro() {
  if (token->kind != TK_IDENT)
    error("generate_define_macro %d got %s", __LINE__, TOKEN_KIND_STR[token->kind]);
  Macro *macro = calloc(1, sizeof(Macro));
  macro->identifier = token;
  token = token->next;

  if (token->kind == TK_LPARE) {
    token = token->next;
    macro->with_param = true;
    Vector *params    = new_vector();
    while(consume(TK_RPARE)) {
      vector_push_back(params, token);
    }
    macro->params = params;

    Vector *replacement = new_vector();
    vector_push_back(replacement, token);
    macro->replacement = get_tokens_until_newline();
    token = token->next;

  } else {
    macro->with_param = false;
    macro->replacement = get_tokens_until_newline();
  }

  vector_push_back(env->macros, macro);
}

void generate_macro() {
  if (token->kind == TK_DEFINE) {
    token = token->next;
    generate_define_macro();
  } else {
    error("unsupported directive %s", token->str);
  }
}

void apply_macro(Macro *macro) {
  Vector *replacement = macro->replacement;
  for (int i = 0; i < replacement->len; i++) {
    emit(vector_get(replacement, i));
  }
  token = token->next;
}

bool apply_if_defined() {
  if (token->kind != TK_IDENT)
    return false;
  for (int i = 0; i < env->macros->len; i++) {
    Macro *macro = vector_get(env->macros, i);
    Token *identifier = macro->identifier;
    if (identifier->len == token->len) {
      if (0 == strncmp(identifier->str, token->str, token->len)) {
        apply_macro(macro);
        return true;
      }
    }
  }
  return false;
}

Vector *preprocess() {
  g_vec = new_vector();

  env = new_env();
  while(token) {
    if (token->kind == TK_NEWLINE) {
      token = token->next;
      continue;
    }
    if (consume(TK_SHARP)) {
      generate_macro();
      continue;
    }
    if (apply_if_defined()) {
      continue;
    }
    emit(token);
    
    token = token->next;
  }

  return g_vec;
}