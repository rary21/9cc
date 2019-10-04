#include <stdio.h>
#include <stdlib.h>

void foo(int a, int b, int c, int d, int e, int f) {
	printf("%d %d %d %d %d %d\n", a, b, c, d, e, f);
}

void alloc4(int** a, int b, int c, int d, int e) {
	long int li;
	li = (long int)b + c;
	*a = malloc(4 * 8);
	(*a)[0] = b;
	(*a)[1] = c;
	(*a)[2] = d;
	(*a)[3] = e;
}