#include <stdio.h>

extern int switch_table(int, char**);
extern int switch_table_2(int, char**);

int main(int argc, char** argv)
{
	printf("testing switch tables in shared library\n");
	switch_table(argc, argv);
	switch_table_2(argc, argv);
}
