/*
@TEST_SCRIPT ./runprog.sh

@GOOD_ARGS_1 "Makefile"
@NORMAL_OUTPUT_CONTAINS_1 -rw

@GOOD_ARGS_2 "-a"
@NORMAL_EXIT_CODE_2 1

@GOOD_ARGS_3 "Makefile;uname"
@NORMAL_EXIT_CODE_3 1

@BAD_ARGS_1 "\$(printf \$BADCOMMAND)a"
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS_1 \.\.

@BAD_ARGS_2 "\`printf \$BADCOMMAND\`a"
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS_2 \.\.


*/
/* This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of their
 * official duties. Pursuant to title 17 Section 105 of the United States
 * Code this software is not subject to copyright protection and is in the
 * public domain. NIST assumes no responsibility whatsoever for its use by
 * other parties, and makes no guarantees, expressed or implied, about its
 * quality, reliability, or any other characteristic.

 * We would appreciate acknowledgement if the software is used.
 * The SAMATE project website is: http://samate.nist.gov
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/*
	One of the most basic filtering, remove the ';' and '-'
*/
void purify(char *__buff)
{
	char buf[512]="";
	char *c = __buff, *b = buf;
	for (;*c != '\0';c++)
	{
			while (*c == ';' || (*c=='-' && *(c+1)!=' ' && *(c+1)!='l') ) c++;
			*b++ = *c;
	}
	strncpy(__buff, buf, 512);
}


int main(int argc, char *argv[]){

	char sys[512]="/bin/ls -l ";
	strncat(sys,argv[1],500);	

	printf("prepur: %s\n",sys);
	purify(sys);
	printf("pospur: %s\n",sys);

        if (0 != (system(sys))){
               printf ("System command failed \n");	
	      exit(1);
        }

	exit(0);
}
