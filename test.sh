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
    int a;
    a = 3;
    return a;
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

# function call
try 55 \
"
  int sum(int a) {
    int b;
    int i;
    b = 0;
    for (i=1; i< a+1;i=i+1)
      b = b+i;
    return b;
  }
  int main() {
    int a;
    int b;
    a = sum(10);
    b = 3;
    while (a < 3*b) {
      a = a+1;
    }
    while (3*a > b) {
      b = b+1;
    }
    a = sum(10);
    return a;
  }
"

#
try 100 \
"
  int main() {
    int a;
    int b;
    a = 10;
    b = &a;
    *b = 100;

    return a;
  }
"
echo OK