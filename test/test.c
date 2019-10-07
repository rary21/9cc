#include <stdio.h>

int a;
int b[3];
int *c;
int main() {
	a = 0;
	a = b[1];
	c = b;
	{
		a = 1;
		int a;
		a = 2;
	}

}
