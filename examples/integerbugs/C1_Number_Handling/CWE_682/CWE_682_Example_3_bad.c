/*
Description Summary
The software performs a calculation that generates incorrect or unintended results that are later used in security-critical decisions or resource management. 

Extended Description
When software performs a security-critical calculation incorrectly, it might lead to incorrect resource allocations, incorrect privilege assignments, or failed comparisons among other things. Many of the direct results of an incorrect calculation can lead to even larger problems such as failed protection mechanisms or even arbitrary code execution. 

@BAD_ARGS 3
@ATTACK_SUCCEEDED_CODE 139


Example 3

This example, taken from CWE-462, attempts to calculate the position of the second byte of a pointer. 

(Bad Code)Example Language: C 
*/

//bme:expanading exmaple to be functional

#include<stdlib.h>

int main(int argc, char** argv){

	int p =  atoi(argv[1]);
	char *second_char;
	second_char = (char *)(p+1);

	printf("We know p is %d\n", p);
	printf("We found out that the second character of p is %c.\n",*second_char);
}



/*
In this example, second_char is intended to point to the second byte of p. But, adding 1 to p actually adds sizeof(int) to p, giving a result that is incorrect (3 bytes off on 32-bit platforms). If the resulting memory address is read, this could potentially be an information leak. If it is a write, it could be a security-critical write to unauthorized memory-- whether or not it is a buffer overflow. Note that the above code may also be wrong in other ways, particularly in a little endian environment.

*/
