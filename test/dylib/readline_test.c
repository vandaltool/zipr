#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {
	printf("Type your answer: ");
	char *answer = readline(NULL);
	printf("%s is wrong.\n", answer);
	return 0;
}

