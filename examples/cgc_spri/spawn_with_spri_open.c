#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>



int main( int argc, char* argv[])
{
	char* spri_file=getenv("STRATA_SPRI_FILE");
	if(spri_file)
	{
		int fd=open(spri_file, O_RDONLY);
		if(fd==-1)
		{
			perror(__FUNCTION__);
		}
		int fd2=dup2(fd,990);
		if(fd2==-1)
		{
			perror(__FUNCTION__);
		}
		close(fd);
	}
	char* exe=getenv("SPAWNER_EXE_FILE");
	if(!exe)
	{
		fprintf(stderr,"Cannot find file to spawn.");
	}
	execvp(exe, argv);
}
