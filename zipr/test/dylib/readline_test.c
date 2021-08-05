#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {
	printf("Type your answer: ");
	char *answer = readline(nullptr);
	printf("%s is wrong.\n", answer);
	return 0;
}

