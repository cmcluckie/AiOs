/* A program that takes a little of musl: printf with a number and a string. */
#include <stdio.h>
int main(void)
{
	printf("%s %d\n", "hello", 42);
	return 0;
}
