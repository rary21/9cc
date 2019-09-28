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
try 0 "20 +  5-  25"
try 24 "12  -24  + 36"
# mul and div
try 13 "13 *4 / 4"
try 6 "52/ 26 *  3"
# all arithmetic calculation
try 21 "78 - 34 * 2 + 22/2"
try 43 "21*2 + 24/6 - 3"
# with parenthesese
try 0 "(2+2) * 4 / 8 - 2"
try 15 "(4 * 2 - 8) / 2 + 3 * (2 + 3)"
# unary
try 3 "-3 + (14+2) * 4 / 8 - 2"
try 25 "+10 + (4 * 2 - 8) / 2 + 3 * (2 + 3)"
# equality and relational
try 1 "3 * 4 == (2 + 10)"
try 1 "3 * 4 >= (2 + 10)"
try 0 "3 * 4 >  (2 + 10)"

echo OK