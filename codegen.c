#include "9cc.h"

const char* regs[] = {"rax", "r10", "rbx", "r12", "r13", "r14", "r15"};
const char* regs8[] = {"al", "r10b", "bl", "r12b", "r13b", "r14b", "r15b"};
const char* regs32[] = {"eax", "r10d", "ebx", "r12d", "r13d", "r14d", "r15d"};

const char* argregs[MAX_ARGS] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen_lval(Node *node) {
  if (node->kind == ND_IDENT) {
    printf("  mov rax, rbp        # start generate lvalue, offset:%d type:%d\n", node->offset, node->type->ty);
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax            # end generate lvalue,   offset:%d\n", node->offset);
  } else if (node->kind == ND_DEREF) {
    gen(node->lhs);  // node->lhs is ND_IDENT, so we just push value of it.
  } else {
    error("gen_lval got %s", NODE_KIND_STR[node->kind]);
  }
}

void gen(Node *node) {
  int i_block = 0;
  int i_arg = 0;

  if (node == NULL)
    return ;

  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_ADDR:
      gen_lval(node->lhs);
      return;
    case ND_DEREF:
      gen(node->lhs);
      printf("  pop r10\n");
      printf("  mov rax, [r10]\n");
      printf("  push rax\n");
      return;
    case ND_IDENT:
      gen_lval(node);
      printf("  pop r10\n");
      printf("  mov rax, [r10]\n");
      printf("  push rax\n");
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen(node->rhs);
      printf("  pop r10\n");
      printf("  pop rax\n");
      printf("  mov [rax], r10      # assign rval to lval\n");
      printf("  push r10            # save assignment result\n");  // assignment can be concatenated
      return;
    case ND_RETURN:
      // ND_RETURN has lhs only
      gen(node->lhs);
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
      printf("  push 0              # tmp\n");
      printf("  je %s\n", node->label_e);
      gen(node->statement);
      printf("  pop rax\n"); // discard previous value
      printf("  jmp %s\n", node->label_s);
      printf("%s:\n", node->label_e);
      return;
    case ND_FOR:
      gen(node->init);
      printf("  pop rax\n"); // discard previous value
      printf("%s:\n", node->label_s);
      gen(node->condition);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  push 0              # tmp\n");
      printf("  je %s\n", node->label_e);
      printf("  pop rax             # tmp\n");
      gen(node->statement);
      printf("  pop rax\n"); // discard previous value
      gen(node->last);
      printf("  pop rax\n"); // discard previous value
      printf("  jmp %s\n", node->label_s);
      printf("%s:\n", node->label_e);
      return;
    case ND_BLOCK:
      printf(" # start of block\n");
      while(node->block[i_block]) {
        gen(node->block[i_block++]);
        printf("  pop rax\n"); // discard previous value
      }
      printf("  push rax            # tmp\n");
      printf(" # end of block\n");
      return;
    case ND_FUNC_CALL:
      while (node->args_call[i_arg]) {
        gen(node->args_call[i_arg]);
        printf("  pop rax\n");
        printf("  mov %s, rax\n", argregs[i_arg++]);
      }
      printf("  call %s\n", node->func_name);
      printf("  push rax\n");
      return;
    case ND_FUNC_DEF:
      printf("%s:\n", node->func_name);
      printf("# start of function\n");
      printf("  push rbp\n");
      printf("  mov rbp, rsp\n");
      printf("  sub rsp, 208\n");
      // get all arguments
      while (node->args_def[i_arg]) {
        if (node->args_def[i_arg]->kind == ND_IDENT) {
          gen_lval(node->args_def[i_arg]);
          printf("  pop rax\n");
          printf("  mov [rax], %s            # retrieve function argument\n", argregs[i_arg++]);
        } else {
          error("function definition has arguments not ident");
        }
      }
      gen(node->body);
      printf("# end of function\n");
      return;
  }

  gen(node->lhs);
  gen(node->rhs);
  char *lreg = regs[1];
  char *rreg = regs[1];
  fprintf(stderr, "kind: %s\n", NODE_KIND_STR[node->kind]);
  if (is_32(node->lhs)) {
    lreg = 1;
  }
  printf("  pop %s\n", regs[1]);
  printf("  pop %s\n", regs[0]);
  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, r10\n");
      break;
    case ND_SUB:
      printf("  sub rax, r10\n");
      break;
    case ND_MUL:
      printf("  imul rax, r10\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv r10\n");
      break;
    case ND_EQ:
      printf("  cmp rax, r10\n");
      printf("  sete al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_NE:
      printf("  cmp rax, r10\n");
      printf("  setne al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LT:
      printf("  cmp rax, r10\n");
      printf("  setl al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, r10\n");
      printf("  setle al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_GT:
      printf("  cmp rax, r10\n");
      printf("  setg al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_GE:
      printf("  cmp rax, r10\n");
      printf("  setge al\n");
      printf("  movzx rax, al\n");
      break;
    default:
      error("error in gen kind: %s", NODE_KIND_STR[node->kind]);
  }
  printf("  push rax\n");
}