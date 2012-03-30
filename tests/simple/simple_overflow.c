

#include <stdio.h>
#include <stdlib.h>

int i;

void overflow(char *in)
{
    char buf[5];
    i = 0;

    
    while(in[i] != '\n' && in[i] != '\0')
    {
	buf[i] = in[i];
	i++;
    }

    buf[i] = '\0';
    printf("%s\n",buf);
}

int main(int argc, char *argv[])
{
    if(argc != 2)
	return 1;

    overflow(argv[1]);
    printf("Finished\n");
    exit(64);
    return 0;
}
