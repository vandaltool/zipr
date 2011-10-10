/*
Description Summary
The software performs a calculation that generates incorrect or unintended results that are later used in security-critical decisions or resource management. 

Extended Description
When software performs a security-critical calculation incorrectly, it might lead to incorrect resource allocations, incorrect privilege assignments, or failed comparisons among other things. Many of the direct results of an incorrect calculation can lead to even larger problems such as failed protection mechanisms or even arbitrary code execution. 

@BAD_ARGS 2000
@ATTACK_SUCCEEDED_CODE 139

*/ 

/*Example 1

The following image processing code allocates a table for images.

(Bad Code)Example Language: C 
*/

#include<stdlib.h>
//#include<iostream>

int main(int argc, char** argv){

	int bank[1000];
	int user = atoi(argv[1]);
	int account = user*2+5; //calculation is wrong if input is too negative or too high
//	bank[account] = 1234;
	
	printf("You are trying to access user %d, his account is number %d\n",user,account);
	printf("The balance is $%d\n",bank[account]);
}
//...
/*
This code intends to allocate a table of size num_imgs, however as num_imgs grows large, the calculation determining the size of the list will eventually overflow (CWE-190). This will result in a very small list to be allocated instead. If the subsequent code operates on the list as if it were num_imgs long, it may result in many types of out-of-bounds problems (CWE-119). 

*/
