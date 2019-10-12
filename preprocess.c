// #include "9cc.h"
// 
// typedef struct Macro Macro;
// struct Macro {
//   Token *token;
//   int len;
// };
// 
// typedef struct Env Env;
// struct Env {
//   Macro *macros;
// };
// 
// Env* new_env(Env *parent) {
//   Env *env = calloc(1, sizeof(Env));
//   env->macros = NULL;
//   return env;
// }
// 
// void define_macro(Token *cur) {
//   if (cur->kind != TK_SHARP)
//     error("define_macro");
//   cur = cur->next;
// 
//   if (cur->kind == TK_IDENT && strncmp(cur->str, "define", cur->len)) {
//     cur = cur->next;
//     new_macro
//   } else {
//     error("unsupported directive %s", cur->str);
//   }
// 
// }
// 
// bool is_macro_defined(Token *cur) {
// 
// }
// 
// void preprocess() {
//   Token *cur;
// 
//   cur = token;
//   do {
//     if (cur->kind == TK_SHARP) {
//       define_macro(cur);
//     }
//     if (cur->kind == TK_IDENT)
//     if (is_macro_defined(cur)) {
// 
//     }
//   } while (cur = cur->next);
// 
// }