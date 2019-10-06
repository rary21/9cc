#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  if [ $? = 0 ]; then
    gcc -g -c -o tmp.o tmp.s
    gcc -g -c -o ./test/foo.o test/foo.c
    gcc -o tmp tmp.o ./test/foo.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
      echo "$input => $actual"
    else
      echo "$input => $expected expected, but got $actual"
      exit 1
    fi
  else
    echo "error in 9cc"
    exit 1
  fi
}

# function call
try 3 \
"
  int bar() {
    int aaa;
    aaa = 3;
    return aaa;
  }
  int main() {
    int a;
    int i;
    a =0;
    for (i=0;i<10;i=i+1)
      a = a+i+1;
    a = bar();
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
    int a[10];
    int *b;
    a[0] = 10;
    a[1] = 20;
    b = a;
    return a[0] + a[1];
  }
"
echo OK