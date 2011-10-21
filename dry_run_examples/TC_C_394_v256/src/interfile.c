#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "interfile.h"

unsigned long readHolidays(int year) {//STONESOUP:unsigned_long
	int *ptr = &year;//STONESOUP:address_as_a_variable
	char holidayFileName[22] = "testData/holiday.";
	char yearAsString[5];
	FILE *holidayf = NULL;
	char line[80];
	int i = 0;


	if (*ptr > 9999) {

		printf("\nERROR: Cannot handle more than 4 digit years");
		exit(1);
	}
	sprintf(yearAsString, "%4d", *ptr);
	yearAsString[4] = '\0';
	strncat(holidayFileName, (const char*) yearAsString, 4);
	holidayFileName[21] = '\0';
	holidayf = fopen(holidayFileName, "r");//STONESOUP:CROSSOVER_POINT
	if (holidayf == NULL) {
		//printf("\nERROR: No holiday file %s for year %d\n", holidayFileName, year);
		return -1;//STONESOUP:TRIGGER_POINT
	}

	while (fgets(line, 80, holidayf) != NULL) {
		// make it unix friendly
		int placeForNull = indexOf(line, '\r', 80, 0);
		if (placeForNull == -1) {
			placeForNull = indexOf(line, '\n', 80, 1);
		}
		line[placeForNull] = '\0';
		holidays[i++] = newDateFromString(line);
	}
	fclose(holidayf);

	return ((unsigned long)i);
}
