#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "must specify at least one argument\n");
	return 1;
  }

  char command[2048];
  sprintf(command, "/bin/ls %s", argv[1]);
  int ret = system(command);

  fprintf (stdout, "%s returned with code: %d\n", command, ret);
}
