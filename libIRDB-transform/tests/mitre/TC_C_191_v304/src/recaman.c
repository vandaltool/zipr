/*******************************************
**
** Copyright (C) 2011 The MITRE Corporation. ALL RIGHTS RESERVED
**
** Author: Mitchell Wills
** Date: 6/28/2011
**
** Base Test Program -- recaman.c
**
** Variant Test Case Program
**
**
** The original base was altered so that when calculating a buffer index the
** number underflows causing the final buffer index to be negative
**
** STONESOUP Weakness Class: Number Handling
** CWE ID: CWE-191
** Variant Spreadsheet Rev #: 2
** Variant Spreadsheet ID: 304
**
** Variant Features:
**   Source Taint: PIPE
**   Data Type:    SIGNED_SHORT
**   Control Flow: SEQUENCE
**   Data Flow:    BUFFER_ADDRESS_ARRAY_INDEX
**
** Initial Testing: (x means yes, - means no)
**   Tested in MS Windows XP 32bit        x
**   Tested in MS Windows 7  64bit        -
**   Tested in Ubuntu10_10 Linux 32bit    -
**   Tested in Ubuntu10_10 Linux 64bit    -
**
** Workflow:
**   Created:6/28/2011
**   1st Vett: <programmer> on <date>
**   2nd Vett: <peer> on <date>
**   3rd Vett: <teamleader> on <date>
**   4th Vett: Fortify Issues Fixed on <date>
**   5th Vett: Tested in Harness on <date>
**
**
** I/0 Pairs:
**   Good: 1st Set:
**         2nd Set:
**         3rd Set:
**         4th Set:
**         5th Set:
**    Bad: 1st Set:
*********************************************/

//Recaman's Sequence Generator
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

//Function prototypes
int mygeti (int *);
int recaman(int);
int inSequence(int, int *, int);

//Securely read in the input number
//Buff[7] means that a maximum of 6 digits can be read
//Any input with a 6 digits in the beginning is valid
//e.g. 123456fffff is valid but ff123456 and 1234ff6234234 are not

int main(void)
{
	printf("*************************************\n");
	printf("*** Recaman's Sequence Generator  ***\n");
	printf("*************************************\n\n");

	short value;
	fputs("Enter a sequence number [ >= 0 ]: ", stdout);
	fflush(stdout);

	fputs("A0", stdout);
	fflush(stdout);

	char *end, buff [7];
	char* rBuff = fgets(buff, sizeof buff, stdin);//STONESOUP:INTERACTION_POINT    //STONESOUP:feature-PIPE
	if(!rBuff){//reached EOF
		value = -1;
		return 1;
	}
	fputs("A", stdout);
	fflush(stdout);
	value = (short) strtol(buff, &end, 10);
	fflush(stdin);
	if(!(!isspace(*buff) && end != buff && (*end == '\n' || *end == '\0')))
		return 1;

	fputs("B", stdout);
	fflush(stdout);

	//-1 is the "magic" value that will quit out of the while loop
	if (value == -1) {
		return 1;
	}
	//Otherwise, send the value to the recaman function if its greater than zero
	else if (value >= 0) {
	fputs("C", stdout);
	fflush(stdout);
		signed short stuff[100];//STONESOUP:feature-SIGNED_SHORT

		short y = (short)(-(-32700-value))/(short)350;//STONESOUP:CROSSOVER_POINT
		short x[2]={0, y};

		*(stuff+x[1]) = value;//STONESOUP:TRIGGER_POINT   //STONESOUP:feature-BUFFER_ADDRESS_ARRAY_INDEX

// CLARK: if you uncomment the printf below, we will get a successful detection
		printf("debug: value: %d %d\n", value, *(stuff+x[1])); // this causes the detection

		printf("%d\n\n", recaman(*(stuff+x[1])));
	}
	else {
		printf("Error: Please enter zero or a positive integer.\n\n");
	}

	return 0;
}

//Calculate the actual sequence
//A(0) = 0. a(m) = a(m-1) - 1 if a(m) is positive and not already in the sequence, otherwise a(m) = a(m-1) + m.
//The first few numbers in the Recaman's Sequence is 0, 1, 3, 6, 2, 7, 13, 20, 12, 21, 11, 22, 10, 23, 9.
int recaman(int sequenceNo)
{
	int *sequence = NULL;
	int i;
	int currentValue;
	int outputValue = 0;

printf("sequenceNo = %d\n", sequenceNo);

	//Create the dynamic array for storing the sequence
	sequence = (int*) calloc((sequenceNo+1),sizeof(int));

	//Check to make sure the calloc call succeeded
	if (sequence == NULL) {
		printf("ERROR: Calloc memory allocation failed\n");
		return -1;
	}

	//Setup the sequence precondition
	//a(0) = 0
	sequence[0] = 0;

	for (i = 1; i <= sequenceNo; i++) {
		currentValue = (sequence[i-1] - i);

		//Do the > 0 check:
		if(currentValue > 0) {
			//Check if the value has already been seen
			if (!inSequence(currentValue, sequence, sequenceNo)) {
				//Add it to the sequence if not
				sequence[i] = currentValue;
			}
			else {
				//Otherwise, do the addition method and add it to the sequence
				sequence[i] = (sequence[i-1] + i);
			}
		}
		//If the value is <0 then do the addition method and add it to the sequence
		else {
			sequence[i] = (sequence[i-1] + i);
		}
	}

	//Set the output value
	outputValue = sequence[sequenceNo];

	//Free the dynamic array
	free(sequence);
	sequence = NULL;

	return outputValue;
}

//See if the input value is already in the sequence
int inSequence(int value, int *seq, int size)
{
	int j;

	for (j=0; j > -size; j--) {
		if (seq[-j] == value) {
			return 1;
		}
	}

	return 0;
}
