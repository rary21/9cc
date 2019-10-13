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

// go to next token and return true if current token has same kind
// otherwise, throw error and exit application
static void expect(TokenKind kind) {
  if (token->kind != kind)
    error("%s expected, but got %s", TOKEN_KIND_STR[kind], TOKEN_KIND_STR[token->kind]);
  // next token
  token = token->next;
}

void generate_define_macro() {
  if (token->kind != TK_IDENT)
    error("generate_define_macro %d got %s", __LINE__, TOKEN_KIND_STR[token->kind]);
  Macro *macro = calloc(1, sizeof(Macro));
  macro->identifier = token;
  token = token->next;

  Token *identifier;
  if (token->kind == TK_LPARE) {
    token = token->next;
    macro->with_param = true;
    Vector *params    = new_vector();
    while(consume(TK_RPARE)) {
      vector_push_back(params, token);
    }
    macro->params = params;

    macro->replacement = token;
    Vector *replacement = new_vector();
    vector_push_back(replacement, token);
    macro->replacement = replacement;

  } else {
    macro->with_param = false;
    if (token->kind != TK_IDENT)
      error("generate_define_macro %d got %s", __LINE__, TOKEN_KIND_STR[token->kind]);
    macro->identifier = token;
    token = token->next;
    if (token->kind != TK_IDENT)
      error("generate_define_macro %d got %s", __LINE__, TOKEN_KIND_STR[token->kind]);
    Vector *replacement = new_vector();
    vector_push_back(replacement, token);
    macro->replacement = replacement;
    token = token->next;
  }

  vector_push_back(env->macros, macro);
}

void generate_macro() {
  if (token->kind == TK_IDENT && strncmp(token->str, "define", token->len)) {
    token = token->next;
    generate_define_macro();
  } else {
    error("unsupported directive %s", token->str);
  }
}

bool is_macro_defined() {
  if (token->kind != TK_IDENT)
    return false;

}

Vector *preprocess() {
  Vector *vec = new_vector();

  env = new_env();
  while(token) {
    // if (consume(TK_SHARP)) {
    //   generate_macro();
    // }
    // if (is_macro_defined()) {

    // }
    vector_push_back(vec, token);
    
    token = token->next;
  }

  return vec;
}