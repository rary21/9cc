#include "9cc.h"

void gen_lval(Node *node) {
  printf("  mov rax, rbp        # start generate lvalue, offset:%d\n", node->offset);
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax            # end generate lvalue,   offset:%d\n", node->offset);
}

void gen(Node *node) {
  int i_block = 0;

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
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen(node->rhs);
      printf("  pop rdi\n");
      printf("  pop rax\n");
      printf("  mov [rax], rdi      # assign rval to lval\n");
      printf("  push rdi\n");  // assignment can be concatenated
      return;
    case ND_RETURN:
      // ND_RETURN has rhs only
      gen(node->rhs);
      printf("  pop rax\n");
      printf("  mov rsp, rbp\n");
      printf("  pop rbp\n");
      printf("  ret\n");
      return;
    case ND_IF:
      gen(node->condition);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je %s\n", node->label_e);
      gen(node->if_statement);
      printf("  jmp %s\n", node->label_s);
      printf("%s:\n", node->label_e);
      gen(node->else_statement);
      printf("%s:\n", node->label_s);
      return;
    case ND_WHILE:
      printf("%s:\n", node->label_s);
      gen(node->condition);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je %s\n", node->label_e);
      gen(node->statement);
      printf("  pop rax\n"); // discard previous value
      printf("  jmp %s\n", node->label_s);
      printf("%s:\n", node->label_e);
      return;
    case ND_FOR:
      gen(node->init);
      printf("%s:\n", node->label_s);
      gen(node->condition);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je %s\n", node->label_e);
      gen(node->statement);
      printf("  pop rax\n"); // discard previous value
      gen(node->last);
      printf("  jmp %s\n", node->label_s);
      printf("%s:\n", node->label_e);
      return;
    case ND_BLOCK:
      printf("# start of block\n");
      while(node->block[i_block]) {
        gen(node->block[i_block++]);
        printf("  pop rax\n"); // discard previous value
      }
      printf("# end of block\n");
      return;
  }

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
    default:
      error("error in gen kind: %s", NODE_KIND_STR[node->kind]);
  }
  printf("  push rax\n");
}