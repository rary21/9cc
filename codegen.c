#include "9cc.h"

const char* regs[] = {"rax", "r10", "rbx", "r12", "r13", "r14", "r15"};
const char* regs8[] = {"al", "r10b", "bl", "r12b", "r13b", "r14b", "r15b"};
const char* regs32[] = {"eax", "r10d", "ebx", "r12d", "r13d", "r14d", "r15d"};

const char* argregs[MAX_ARGS] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
const char* argregs8[MAX_ARGS] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
const char* argregs32[MAX_ARGS] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};

const char* get_reg(int i_reg, Node *node) {
  if (is_32(node))
    return regs32[i_reg];
  if (is_8(node))
    return regs8[i_reg];
  return regs[i_reg];
}

const char* get_fncdef_reg(int i_arg, Node *node) {
  if (is_32(node))
    return argregs32[i_arg];
  if (is_8(node))
    return argregs8[i_arg];
  return argregs[i_arg];
}

void gen_lval(Node *node) {
  if (node->kind == ND_IDENT) {
    if (node->var->is_global) {
      printf("  lea rax, %s[rip]            # generate global lvalue\n", node->name);
      printf("  push rax\n");
    } else {
      int offset = node->var->offset;
      printf("  mov rax, rbp        # start generate lvalue, offset:%d type:%d\n", offset, node->type->ty);
      printf("  sub rax, %d\n", offset);
      printf("  push rax            # end generate lvalue,   offset:%d\n", offset);
    }
  } else if (node->kind == ND_LITERAL) {
    printf("  lea rax, .LC%d            # generate literal\n", node->literal_id);
    printf("  push rax\n");
  } else if (node->kind == ND_ARG_DECL) {
    int offset = node->var->offset;
    printf("  mov rax, rbp        # start generate lvalue, offset:%d type:%d\n", offset, node->type->ty);
    printf("  sub rax, %d\n", offset);
    printf("  push rax            # end generate lvalue,   offset:%d\n", offset);
  } else if (node->kind == ND_DEREF) {
    gen(node->lhs);  // node->lhs is ND_IDENT, so we just push value of it.
  } else if (node->kind == ND_DOT) {
    gen_lval(node->lhs);
    printf("  pop rax\n");
    printf("  add rax, %d\n", node->type->offset);
    printf("  push rax\n");
  } else if (node->kind == ND_ARROW) {
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  add rax, %d\n", node->type->offset);
    printf("  push rax\n");
  } else {
    error("gen_lval got %s", NODE_KIND_STR[node->kind]);
  }
}

void gen(Node *node) {
  int i_block = 0;
  int i_arg = 0;
  int alignment = 0;

  if (node == NULL)
    return ;

  debug_print("kind: %s ", NODE_KIND_STR[node->kind]);
  if (node->lhs)
    debug_print("l:%s ", NODE_KIND_STR[node->lhs->kind]);
  if (node->rhs)
    debug_print("r:%s", NODE_KIND_STR[node->rhs->kind]);

  debug_put("\n");

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
    case ND_DOT:
      gen_lval(node->lhs);
      printf("  pop r10\n");
      printf("  mov rax, [r10 + %d]\n", node->type->offset);
      printf("  push rax\n");
      return;
    case ND_ARROW:
      gen(node->lhs);
      printf("  pop r10\n");
      printf("  mov rax, [r10 + %d]\n", node->type->offset);
      printf("  push rax\n");
      return;
    case ND_IDENT:
      gen_lval(node);
      // generate address for array, otherwise get content of it
      if (node->type->ty != ARRAY) {
        printf("  pop r10\n");
        printf("  mov rax, [r10]\n");
        printf("  push rax\n");
      }
      return;
    case ND_LITERAL:
      gen_lval(node);
      return;
    case ND_ASSIGN:
    case ND_VAR_INIT:
      gen_lval(node->lhs);
      gen(node->rhs);
      const char* reg = get_reg(1, node->rhs);
      printf("  pop r10\n");
      printf("  pop rax\n");
      printf("  mov [rax], %s       # assign rval to lval\n", reg);
      printf("  push r10            # save assignment result\n");  // assignment can be concatenated
      return;
    case ND_POSTINC:
      gen(node->rhs); // generate current value
      gen(node->statement);
      printf("  pop rax\n"); // discard incremented result
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
      printf("  push 0              # tmp\n");
      printf("  je %s\n", node->label_e);
      printf("  pop r10             # tmp\n");
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
      for (int i = 0; i < node->block->len; i++) {
        gen(vector_get(node->block, i));
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
      printf("  mov eax, 0     # al represents number of float argument.\n");
      printf("  call %s\n", node->func_name);
      printf("  push rax\n");
      return;
    case ND_FUNC_DEF:
      printf("%s:\n", node->func_name);
      printf("# start of function\n");
      printf("  push rbp\n");
      printf("  mov rbp, rsp\n");
      int lsize = node->locals_size;
      // ensure rsp 16 bytes alignment
      if ((lsize % 16) != 0) {
        lsize = lsize + (16 - lsize % 16);
      }
      printf("  sub rsp, %d\n", lsize);
      // get all arguments
      while (node->args_def[i_arg]) {
        if (node->args_def[i_arg]->kind == ND_ARG_DECL) {
          gen_lval(node->args_def[i_arg]);
          const char* reg = get_fncdef_reg(i_arg, node->args_def[i_arg]);
          printf("  pop rax\n");
          printf("  mov [rax], %s      # retrieve function argument\n"
                 , reg);
          i_arg++;
        } else {
          error("function definition has arguments not ident %s\n", NODE_KIND_STR[node->args_def[i_arg]->kind]);
        }
      }
      gen(node->body);
      printf("# end of function\n");
      return;
    case ND_GVAR_DECL:
      alignment = 32; // TODO: compute best alignment
      printf("  .data\n");
      if (node->init_list) {
      printf("  .type %s,@object\n", node->name);
      printf("  .size %s,%d\n", node->name, node->type->size);
      printf("  .align %d\n", alignment);
      if (node->type->is_const)
        printf("  .section .rodata\n");
      printf("%s:\n", node->name);
        char* size = ".quad";
        if (is_32_elem(node))
          size = ".long";
        else if (is_8_elem(node))
          size = ".byte";
        for (int i = 0; i < node->init_list->len; i++) {
          Node *init = vector_get(node->init_list, i);
          printf("  %s ", size);
          if (init->kind == ND_NUM) {
            printf("%d\n", init->val);
          } else if (init->kind == ND_LITERAL) {
            printf(".LC%d\n", init->literal_id);
          }
        }
      } else {
        printf("  .comm %s,%d,%d\n", node->name, node->type->size, alignment);
      }
      printf("  .text\n");
      return;
  }

  gen(node->lhs);
  gen(node->rhs);
  printf("  pop %s\n", regs[1]);
  printf("  pop %s\n", regs[0]);
  const char *lreg = regs[0];
  const char *rreg = regs[1];
  if (!same_type(node->lhs->type, node->rhs->type) || !same_size(node->lhs, node->rhs))
  {
    debug_print("%d %d\n", node->lhs->type->ty, node->rhs->type->ty);
    debug_print("%s\n", NODE_KIND_STR[node->kind]);
    error("not same type\n");
  }
  if (is_32(node->lhs))
  {
    lreg = regs32[0];
  }
  else if (is_8(node->lhs))
  {
    lreg = regs8[0];
  }
  if (is_32(node->rhs))
  {
    rreg = regs32[1];
  }
  else if (is_8(node->rhs))
  {
    rreg = regs8[1];
  }
  switch (node->kind) {
    case ND_ADD:
      printf("  add %s, %s\n", lreg, rreg);
      break;
    case ND_SUB:
      printf("  sub %s, %s\n", lreg, rreg);
      break;
    case ND_MUL:
      printf("  imul %s, %s\n", lreg, rreg);
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv %s\n", rreg);
      break;
    case ND_EQ:
      printf("  cmp %s, %s\n", lreg, rreg);
      printf("  sete al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_NE:
      printf("  cmp %s, %s\n", lreg, rreg);
      printf("  setne al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LT:
      printf("  cmp %s, %s\n", lreg, rreg);
      printf("  setl al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_LE:
      printf("  cmp %s, %s\n", lreg, rreg);
      printf("  setle al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_GT:
      printf("  cmp %s, %s\n", lreg, rreg);
      printf("  setg al\n");
      printf("  movzx rax, al\n");
      break;
    case ND_GE:
      printf("  cmp %s, %s\n", lreg, rreg);
      printf("  setge al\n");
      printf("  movzx rax, al\n");
      break;
    default:
      error("error in gen kind: %s", NODE_KIND_STR[node->kind]);
  }
  printf("  push rax\n");
}