#include <stdio.h>
int main() {
	int a;
	a = 0;
	{
		a = 1;
		int a;
		a = 2;
	}

}
