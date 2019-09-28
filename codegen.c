#include "9cc.h"

void gen_lval(Node *node) {
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");

}

void gen(Node *node) {
  if (node == NULL)
    return ;
  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_IDENT:
      gen_lval(node);
      printf("  pop rdi\n");
      printf("  mov rax, [rdi]\n");
      printf("  push rax\n");
      return;
  }
  if (node->kind == ND_ASSIGN && node->lhs != NULL && node->lhs->kind == ND_IDENT)
    gen_lval(node->lhs);
  else
    gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");
  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_NE:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LT:
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_GT:
      printf("  cmp rax, rdi\n");
      printf("  setg al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_GE:
      printf("  cmp rax, rdi\n");
      printf("  setge al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_ASSIGN:
      printf("  mov [rax], rdi\n");
      printf("  mov rax, rdi\n");  // assignment can be concatenated
      break;
    default:
      error("error in gen");
  }
  printf("  push rax\n");
}