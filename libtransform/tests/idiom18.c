#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 100

struct foobar {
  int x;
  int y;
};

int foobar(struct foobar *myarray, struct foobar *(myarray2d[2][MAX_SIZE/2]))
{
	int i, j;
	int sum = 0;
	for (i = 0; i < 2; ++i)
	for (j = 0; j < MAX_SIZE/2; ++j)
	{
		(*myarray2d)[i][j] = myarray[i * 2 + j];
		sum += myarray2d[i][j];
	}
	return sum;
}

int main(int argc, char **argv)
{
	struct foobar myarray[MAX_SIZE];
	struct foobar myarray2d[2][MAX_SIZE/2];
	struct foobar* ptr = &myarray[0];
	int i;
	int j;
	int z = 100;
	
	if (argc > 1)
		z = atoi(argv[1]);

	for (i = 0; i < MAX_SIZE; i++)
	{
		ptr[i].x = i;
		ptr[i].y = i+1;
	}

	for (i = 0; i < 2; ++i)
	for (j = 0; j < z/2; ++j)
		myarray2d[i][j] = ptr[i * 2 + j];

	int x = foobar(&myarray, &myarray2d);
	printf("x = %d\n", x);
	return 0;
}
