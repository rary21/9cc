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

# # add and sub
# try 4 "a=1 + 3;"
# try 6 "b0 = 3-2;a= 3 + 2;c = a + b0;"
# # mul and div
# try 9 "a1 = 1 * 3; b = a1 * 3;"
# try 1 "c = 3; c = c /3;"
# # all arithmetic calculation
# try 30 "abd = (1 + 3) * 3; b = (abd + 3) * 2;"
# try 10 "a = (2 -1) * 2; b = (a + a) * 2; c = a + b;"
# # equality and relational
# try 1 "ab = 1;b = 1; ab==b;"
# try 0 "abc = 2 * 2; b = 5; abc > b;"
# # return
# try 30 "abd = (1 + 3) * 3; b = (abd + 3) * 2; return b;"
# try 10 "a = (2 -1) * 2; b = (a + a) * 2; c = a + b; return c;"
# try 1 "ab = 1;b = 1; ab==b;return ab;"
# try 0 "abc = 2 * 2; b = 5; return abc > b;"

# # if statement
# try 4 "a = 0; b = 2; if (a == b) return 3; else return 4;"
# try 4 "a = 1; b = 2; if (a + b < 3) return 3; return 4;"
# try 3 "a = 2; b = 2; if (a == b) return 3; else return 4;"
# # while statement
# try 6 "b=0; while(b < 5) b = b + 2; return b;"
# try 16 "b=1; while(b < 10) b = b * 2; return b;"
# # for statement
# try 55 "b=0; for (a=1; a<=10; a=a+1) b=b+a; return b;"
# try 8 "b=0; for (a=1; a<=10; a=a+b) b=b+a; return b;"
# # block
# try 1 "{b=2; a=1; return a;}"
# try 3 "a = 0; b=0; if (a==b) {a = 1; b=2;} {a = a+b; a=0; a=1;return a + b;}"
# function call
try 55 \
"
  main()
    a =0;
    for (i=0;i<10;i=i+1)
      a = a+i+1;
    foo(a, i, 2, 3, 4, 5);
    return a;
"

echo OK