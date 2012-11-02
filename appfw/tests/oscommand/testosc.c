#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>

char *ls_string="ls";
char *env_string="env";

int main(int argc, char *argv[], char* envp[])
{
	char** envp2=malloc(4*sizeof(void*));
	int i;
	for(i=0;i<3;i++)
		envp2[i]=envp[i];
	envp2[3]=NULL;

  	if (argc < 3)
  	{
    		fprintf(stderr, "must specify at least one argument\n");
		return 1;
  	}


	switch(atoi(argv[1]))
	{
		case 1:  // system
		{
  			char command[2048];
  			sprintf(command, "/bin/ls %s", argv[2]);
  			int ret = system(command);
  			fprintf (stdout, "%s returned with code: %d\n", command, ret);
			break;
		}
		case 2://popen
		{
  			char command[2048];
  			sprintf(command, "/bin/ls %s", argv[2]);
  			FILE* ret = popen(command,"r");
			
  			fprintf (stdout, "%s returned file descriptor: %d\n", command, ret?fileno(ret):0);
			if(ret) pclose(ret);
			break;
		}
		case 3: //rcmd
		{
			char *host="none";
  			char command[2048];
  			sprintf(command, "/bin/ls %s", argv[2]);
  			int ret = rcmd(&host,0,NULL,NULL,command,NULL);
			
  			fprintf (stdout, "%s returned with code: %d\n", command, ret);
			break;
		}
		case 4: 
		{
			FILE* fls=fopen("/bin/ls", "r");
			argv[1]="/bin/ls";
			fexecve(fileno(fls), &argv[1], envp2);
			assert(0);
		}
		case 5: 
		{

			argv[1]="/bin/ls";
			execve("/bin/ls",  &argv[1], envp2);
			assert(0);
		}
		case 6: 
		{
			execl("/bin/ls",  "/bin/ls", argv[2], NULL);
			assert(0);
		}

		case 7: 
		{
			execle("/bin/ls",  "/bin/ls", argv[2], NULL, envp2);
			assert(0);
		}

		case 8: 
		{
			argv[1]="/bin/ls";
			execv("/bin/ls",  &argv[1]);
			assert(0);
		}

		case 9: 
		{
			argv[1]=ls_string;
			execvp(ls_string,  &argv[1]);
			assert(0);
		}
		case 10: 
		{
			argv[1]=ls_string;
			execvpe(ls_string,  &argv[1], envp2);
			assert(0);
		}
		case 11: 
		{
			argv[1]=ls_string;
			execlp(ls_string,  argv[1], argv[2], NULL);
			assert(0);
		}

		case 101:  // system
		{
  			char command[2048];
  			sprintf(command, "/usr/bin/env %s", argv[2]);
  			int ret = system(command);
  			fprintf (stdout, "%s returned with code: %d\n", command, ret);
			break;
		}
		case 102://popen
		{
  			char command[2048];
  			sprintf(command, "/usr/bin/env %s", argv[2]);
  			FILE* ret = popen(command,"r");
			
  			fprintf (stdout, "%s returned file descriptor: %d\n", command, ret?fileno(ret):0);
			if(ret) pclose(ret);
			break;
		}
		case 103: //rcmd
		{
			char *host="none";
  			char command[2048];
  			sprintf(command, "/usr/bin/env %s", argv[2]);
  			int ret = rcmd(&host,0,NULL,NULL,command,NULL);
			
  			fprintf (stdout, "%s returned with code: %d\n", command, ret);
			break;
		}
		case 104: 
		{
			FILE* fls=fopen("/usr/bin/env", "r");
			argv[1]="/bin/ls";
			fexecve(fileno(fls), &argv[1], envp2);
			assert(0);
		}
		case 105: 
		{

			argv[1]="/usr/bin/env";
			execve("/usr/bin/env",  &argv[1], envp2);
			assert(0);
		}
		case 106: 
		{
			execl("/usr/bin/env",  "/bin/ls", argv[2], NULL);
			assert(0);
		}

		case 107: 
		{
			execle("/usr/bin/env",  "/bin/ls", argv[2], NULL, envp2);
			assert(0);
		}

		case 108: 
		{
			argv[1]="/usr/bin/env";
			execv("/usr/bin/env",  &argv[1]);
			assert(0);
		}

		case 109: 
		{
			argv[1]=env_string;
			execvp(env_string,  &argv[1]);
			assert(0);
		}
		case 110: 
		{
			argv[1]=env_string;
			execvpe(env_string,  &argv[1], envp2);
			assert(0);
		}
		case 111: 
		{
			argv[1]=env_string;
			execlp(env_string,  argv[1], argv[2], NULL);
			assert(0);
		}

		default:
		{
			fprintf(stderr, "Cannot parse argv[1]\n");
		}
	}

  return 0;
}
