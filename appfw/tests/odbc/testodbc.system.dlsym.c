#include <stdio.h>
#include <dlfcn.h>

int main(int argc, char **argv)
{
	char *commandInject = getenv("INJECT");
	int (*my_system)(const char *) = 0L;

 	void *libc = dlopen("libc.so.6", RTLD_LAZY);
	if (!libc)
	{
		fprintf(stderr, "couldn't find libc.so: exiting");
		exit(-1);
	}

	my_system = dlsym(libc, "system");
	if (!my_system)
	{
		fprintf(stderr, "couldn't resolve system: exiting");
		exit(-1);
	}

	if (commandInject)
		return (*my_system)(commandInject);
	else
		return (*my_system)(argv[1]);
}
