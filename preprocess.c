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

Vector *get_tokens_until(TokenKind kind) {
  Vector *vec = new_vector();
  while (token->kind != kind) {
    vector_push_back(vec, token);
    token = token->next;
  }
  token = token->next;
  return vec;
}

Vector *get_tokens_until_newline() {
  Vector *vec = get_tokens_until(TK_NEWLINE);
  return vec;
}

Vector *get_arguments_definition() {
  Vector *vec = new_vector();
  while (1) {
    if (token->kind != TK_IDENT) {
      char str[256];
      strncpy(str, token->str, token->len);
      error("\"%s\" may not appear in macro parameter list", str);
    }
    vector_push_back(vec, token);
    token = token->next;
    if (!consume(TK_COMMA)) {
      expect(TK_RPARE);
      break;
    }
  }
  return vec;
}

Vector *get_arguments() {
  Vector *vec = new_vector();
  while (1) {
    vector_push_back(vec, token);
    token = token->next;
    if (!consume(TK_COMMA)) {
      expect(TK_RPARE);
      break;
    }
  }
  return vec;
}

void generate_define_macro() {
  if (token->kind != TK_IDENT)
    error("generate_define_macro %d got %s", __LINE__, TOKEN_KIND_STR[token->kind]);
  Macro *macro = calloc(1, sizeof(Macro));
  macro->identifier = token;
  token = token->next;

  if (consume(TK_LPARE)) {
    macro->with_param = true;
    macro->params = get_arguments_definition();
    macro->replacement = get_tokens_until_newline();
  } else {
    macro->with_param = false;
    macro->replacement = get_tokens_until_newline();
  }

  vector_push_back(env->macros, macro);
}

void generate_macro() {
  if (consume(TK_DEFINE)) {
    generate_define_macro();
  } else {
    error("line:%d unsupported directive %s", get_line_number(token), token->str);
  }
}

int get_param_index(Vector *params, Token *tkn) {
  for (int i = 0; i < params->len; i++) {
    Token *_tkn = vector_get(params, i);
    if (tkn->len != _tkn->len)
      continue;
    if (0 == strncmp(tkn->str, _tkn->str, tkn->len))
      return i;
  }
  return -1;
}

void apply_macro(Macro *macro) {
  Vector *macro_params = macro->params;
  Vector *replacement  = macro->replacement;

  token = token->next;
  if (macro->with_param) {
    expect(TK_LPARE);
    Vector *params = get_arguments();
    if (params->len != macro_params->len) {
      char str[256];
      strncpy(str, macro->identifier->str, macro->identifier->len);
      error("%s expects %d arguments but got %d\n", str,
            macro_params->len, params->len);
    }
    for (int i = 0; i < replacement->len; i++) {
      Token *repl = vector_get(replacement, i);
      int param_index = get_param_index(macro_params, repl);
      if (param_index == -1) {
        // if repl is not in the parameter list
        emit(vector_get(replacement, i));
      } else {
        // replace parameter
        emit(vector_get(params, param_index));
      }
    }
  } else {
    for (int i = 0; i < replacement->len; i++) {
      emit(vector_get(replacement, i));
    }
  }
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

bool apply_special_macro() {
  if (token->kind != TK_IDENT)
    return false;
  if (0 == strncmp(token->str, "__LINE__", 8)) {
    Token *line = calloc(1, sizeof(Token));
    line->kind = TK_NUM;
    line->val  = get_line_number(token);
    emit(line);
    token = token->next;
    return true;
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
    if (apply_special_macro()) {
      continue;
    }
    emit(token);
    
    token = token->next;
  }

  return g_vec;
}