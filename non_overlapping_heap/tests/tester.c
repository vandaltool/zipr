#include <sys/mman.h>
#include <stdio.h>

int main() {
	void* foobar = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	printf("%p\n",foobar);
	return 0;
}
