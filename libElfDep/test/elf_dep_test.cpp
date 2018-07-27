
#include <stdio.h>
extern "C"
{

int elf_dep_test_var=0;

void elf_dep_test_callback()
{
	printf("Elf_dep_test var = %d\n", elf_dep_test_var);
}

}
