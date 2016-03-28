#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>

int main() {
	int fd = open("./inputfile", O_RDONLY);
	void* foobar = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	printf("%p\n",foobar);
	printf("%s\n",(char*)foobar);
	return 0;
}
