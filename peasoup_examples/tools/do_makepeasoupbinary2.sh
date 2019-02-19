#!/bin/sh 


name=$1

current_dir=`pwd`
peasoup_binary=$name.sh


echo "
int main(int argc, char* argv[0], char* envp[])
{

	char *newargv[argc+3];
	const char* ps_run=\"$current_dir/ps_run.sh\";
	newargv[0]=argv[0];
	newargv[1]=\"$current_dir\";
	unsigned int i=0;
	for(i=0;i<argc;i++)
		newargv[i+2]=argv[i];
	newargv[argc+2]=0;

	execve(ps_run,newargv,envp);

	perror(\"Unable to start ps_run.sh\");
	return -1;
}" |  gcc -o $peasoup_binary -w -xc -

cp $PEASOUP_HOME/tools/ps_run.sh $current_dir

