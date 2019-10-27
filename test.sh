#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  if [ $? = 0 ]; then
    gcc -g -c -o tmp.o tmp.s
    gcc -g -c -o ./test/foo.o test/foo.c
    gcc -no-pie -o tmp tmp.o ./test/foo.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
      echo "$input => $actual"
    else
      echo "$input => $expected expected, but got $actual"
      exit 1
    fi
  else
    echo $input
    echo "error in 9cc"
    exit 1
  fi
}

try 0 \
"
int printf();
int printQueen(int *queen)
{
    int i;
    
    printf(\"%d queen : \", 8);
    for (i = 0; i < 8; i = i+1) {
        printf(\"%d \", queen[i]);
    }
    printf(\"\n\");
}

int check(int *queen)
{
    int i;
    int j;
    
    for (i = 0; i < 8-1; i=i+1) {
        for (j = i+1; j < 8; j=j+1) {
            if (queen[i] == queen[j]) return 0;
            if (queen[i] - queen[j] == j - i) return 0;
            if (queen[j] - queen[i] == j - i) return 0;
        }
    }
    return 1;
}


int setQueen(int *queen, int i)
{
    int j;

    if (i == 8) {
        if (check(queen)) printQueen(queen);
        return 1;
    }
    
    for (j = 0; j < 8; j=j+1) {
        queen[i] = j;
        setQueen(queen, i+1);
    }
    return 0;
}

int main()
{
    int queen[8];
    setQueen(queen, 0);
}
"

# function call
try 55 \
"
  int main() {
    int a;
    int i;
    a = 0;
    for (i=0;i<10;i=i+1) {
     int c;
     c = 0;
     a = a+i+1;
   }
   return a;
 }
"

try 3 \
"
  int bar() {
    int aaa;
    aaa = 3;
    return aaa;
  }
  int main() {
    int a;
    a = bar();
    return a;
  }
"

# function call
try 75 \
"
  int sum(int start, int b) {
    int c;
    int d;
    int i;
    i = b;
    d=0;
    for (i=start; i< b+1;i=i+1)
      d = d + i;
    return d;
  }
  int main() {
    int a;
    int b;
    int c;
    a = sum(1, 10);
    b = 3;
    while (a < 3*b) {
      a = a+1;
    }
    while (3*a > b) {
      b = b+1;
    }
    a = sum(10, 15);
    return a;
  }
"


try 6 \
"
  int alloc4();
  int main() {
    int a;
    int *b;
    int *cc;
    int *c;
    alloc4(&cc, 100, 1, 2, 3);
    a = 10;
    b = &a;

    a = *(cc+1);
    return a + *(cc+2) + *(cc+3);
  }
"

try 12 \
"
  int main() {
    int a;
    int *b;
    return sizeof b + sizeof a;
  }
"

try 30 \
"
  int sum(int a) {
    return a;
  }
  int main() {
    int a[30];
    int *b;
    a[0] = 10;
    a[1] = 20;
    b = a;
    return a[0] + a[1];
  }
"

try 55 \
"
  int sum(int a) {
    int cnt;
    int i;
    cnt = 0;
    {
      int cnt;
      int i;
      cnt = 10;
      i = 10;
    }
    for (i = 1; i < a+1; i=i+1) {
      cnt = cnt + i;
    }
    return cnt;
  }
  int main() {
    int a;
    a = sum(10);
    return a;
  }
"

try 55 \
"
  int glob;
  int sum() {
    int cnt;
    int i;
    cnt = 0;
    {
      int cnt;
      int i;
      cnt = 10;
      i = 10;
    }
    for (i = 1; i < glob+1; i=i+1) {
      cnt = cnt + i;
    }
    return cnt;
  }
  int main() {
    glob = 10;
    int a;
    a = sum();
    return a;
  }
"

try 99 \
"
  int a;
  int b[10];
  int *c;
  int main() {
    c = b;
    b[0] = 100;
    int a;
    int b;
    int *c;
    a = 99;
    c = &a;
    return *c;
  }
"

try 0 \
"
  int a;
  int b[100];
  int *c;
  int isbigger(int b) {
    if (b > a) {
      return 1;
    }
    return 0;
  }
  int main() {
    a = 100;
    return isbigger(99);
  }
"

try 1 \
"
  int a;
  int b[100];
  int *c;
  int isbigger(int b) {
    if (b > a) {
      return 1;
    } else {
      return 0;
    }
  }
  int main() {
    a = 100;
    return isbigger(101);
  }
"

try 199 \
"
  char str[100];
  int main() {
    int a;
    str[1] = 99;
    str[0] = 100;
    a = str[0] + str[1];
    return a;
  }
"

try 100 \
"
  char str[10];
  char ccc[10];
  int main() {
    str[0] = 50;
    ccc[0] = 100;
    return ccc[0];
  }
"

try 99 \
"
  char str[100];
  int main() {
    int a;
    int b;
    int c;
    a = 100;
    b = 500;
    c = 5000;
    str[0] = a;
    str[1] = -1;
    str[2] = a;
    str[3] = -3;
    a = str[0] + str[1];
    return a;
  }
"

try 97 \
"
  char *str;
  int main() {
    str = \"abc\";
    int a;
    a = str[0];
    return a;
  }
"

try 98 \
"
  char *str;
  int main() {
    str = \"abc\";
    int a;
    a = str[1];
    return a;
  }
"

try 99 \
"
  char *str;
  int main() {
    str = \"abc\";
    int a;
    a = str[2];
    return a;
  }
"

try 97 \
"
  int printf();
  int a;
  char **pstr;
  char *str;
  int main() {
    pstr = &str;
    str = \"abc\";
    a = 999;
    printf(\"first printf %d\n\", a);
    a = str[2];
    return *pstr[0];
  }
"

try 97 \
"
  #define DEFAULT 13
  int printf();
  int a;
  char **pstr;
  char *str;
  int main() {
    pstr = &str;
    str = \"abc\";
    a = DEFAULT;
    printf(\"first printf %d\n\", a);
    a = str[2];
    return *pstr[0];
  }
"

try 6 \
"
  #define ADD 1 + 2 + 3
  int main() {
    return ADD;
  }
"

try 50 \
"
  #define SUB(a, b) a-b

  int main() {
    int aa = 100;
    int bb = 50;
    return SUB(aa,bb);
  }
"

try 0 \
"
  #define SUB(a, b) a-b

  int main() {
    int aa = 100;
    int bb = aa;
    return SUB(aa,bb);
  }
"

try 0 \
"
  int exit();
  int printf();
  #define EXPECT(expected, expr)                                    \
    {                                                               \
      int e1 = (expected);                                          \
      int e2 = (expr);                                              \
      if (e1 == e2) {                                               \
        printf(\"%d => %d\n\", e1, e2);                             \
      } else {                                                      \
        printf(\"%d expected, but got %d\n\", e1, e2);              \
        exit(1);                                                    \
      }                                                             \
    }

  int main () {
    EXPECT(1, 1)
    return 0;
  }
"

try 0 \
"
  int printf();
  int main () {
    printf(\"%d\\n\", __LINE__);
    printf(\"%d\\n\", __LINE__);
    printf(\"%d\\n\", __LINE__);
    printf(\"%d\\n\", __LINE__);
    printf(\"%d\\n\", __LINE__);
    return 0;
  }
"

try 1 \
" int main () {
    int a = 0;
    int c = 257;
    a = (char)c;
    return a;
  }
"

try 62 \
" int main () {
    int a = 0;
    int c = 30;
    a = ++c;
    return a + c;
  }
"

try 61 \
" int main () {
    int a = 0;
    int c = 30;
    a = c++;
    return a + c;
  }
"

try 12 \
" 
  struct test {
    char c;
    int b;
    char d;
  } a;
  int main () {
    return sizeof(a);
  }
"

try 45 \
" 
  struct test {
    char a;
    int b;
    char c;
  } a;
  int main () {
    struct test ss;
    ss.a + ss.b;
    ss.a = 255;
    ss.b = ss.a;
    ss.b = 300;
    return ss.b - ss.a;
  }
"

try 45 \
" 
  int printf();
  struct test {
    char *str;
    int b;
    char c;
  };
  int main () {
    struct test ss;
    ss.str = \"abc\";
    printf(\"%s\\n\", ss.str);
    ss.b = 300;
    ss.c = 255;
    return ss.b - ss.c;
  }
"

try 45 \
" 
  struct test {
    char *str;
    int b;
    char c;
  };
  int main () {
    struct test ss;
    struct test *ptr;
    ptr = &ss;
    ptr->b = 300;
    ptr->c = 255;
    return ss.b - ss.c; 
  }
"

try 45 \
" 
  struct test {
    char *str;
    int b;
    char c;
  };
  int main () {
    struct test const ss;
    struct test *ptr;
    ptr = &ss;
    ptr->b = 300;
    ptr->c = 255;
    return ss.b - ss.c; 
  }
"

try 45 \
" 
  typedef struct test test;
  struct test {
    char *str;
    int b;
    char c;
  }a;
  int main () {
    a.b = 300;
    test ss;
    test *ptr;
    ptr = &ss;
    ptr->b = 300;
    ptr->c = 255;
    return ss.b - ss.c; 
  }
"

try 100 \
" 
  void *calloc();
  typedef struct test test;
  struct test {
    char *str;
    int b;
    char c;
  }a;
  void* foo() {
    test *a = calloc(1, sizeof(test));
    a->b = 100;
    return a;
  }
  int main () {
    test *a = foo();
    return a->b;
  }
"

try 98 \
" 
  enum test {A=98, B, C} typedef test;
  int main () {
    int C = A;
    return C;
  }
"

try 4 \
" 
  typedef struct Type Type;
  struct Type {
    enum {VOID, INT, CHAR, PTR, ARRAY, STRUCT, ENUM, INVALID} ty;
    struct Type *ptr_to;
    int size;
    int align;
    int array_size;
  };
  int main () {
    Type type;
    type.ty = ARRAY;
    return type.ty;
  }
"

try 10 \
" 
  int printf();
  int main () {
    int a[10] = {1,2,3,4};
    char b[10] = {1,2,3,4};
    const char* str[3] = {\"abc\", \"def\", \"ghi\"};
    printf(\"%s\n\", str[2]);
    return a[3]+b[0]+b[1]+b[2];
  }
"
echo OK