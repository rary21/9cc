#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# add and sub
try 4 "a=1 + 3;"
try 6 "b0 = 3-2;a= 3 + 2;c = a + b0;"
# mul and div
try 9 "a1 = 1 * 3; b = a1 * 3;"
try 1 "c = 3; c = c /3;"
# all arithmetic calculation
try 30 "abd = (1 + 3) * 3; b = (abd + 3) * 2;"
try 10 "a = (2 -1) * 2; b = (a + a) * 2; c = a + b;"
# equality and relational
try 1 "ab = 1;b = 1; ab==b;"
try 0 "abc = 2 * 2; b = 5; abc > b;"

echo OK