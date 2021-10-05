#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
	// Scanf an entire line with memory modifier m, continue only if characters were typed
	printf("Insert message:\n");
	do {
		scanf("%m[^\n]", &buffer);
		fflush(stdin);
	} while (buffer == NULL);
	return 0;
}