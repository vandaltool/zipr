/*******************************************
 **
 ** Copyright (C) 2011 The MITRE Corporation. ALL RIGHTS RESERVED
 **
 ** Author: Erik Landskov
 ** Date: 7/26/2011
 **
 ** Base Test Program -- business.c
 **
 ** Given two dates in the same year, this Base Test Program computes the number of
 ** business days between them.  A business day is described as any day which is not a
 ** weekend day or a holiday.  The dates that are holidays are provided via a file.
 **
 ** Variant Test Case Program
 **
 ** The program is only built to handle dates in the year 2011. Rather then error and exit,
 ** the program now gets stuck in an infinite loop if the inputed dates are not in 2011.
 **
 ** STONESOUP Weakness Class: Error Handling
 ** CWE ID: CWE-394
 ** Variant Spreadsheet Rev #: 3
 ** Variant Spreadsheet ID: 256
 **
 ** Variant Features:
 **   Source Taint:environment_variable
 **   Data Type:unsigned_long
 **   Control Flow:interfile_2
 **   Data Flow:address_as_a_variable
 **
 ** Initial Testing: (x means yes, - means no)
 **   Tested in MS Windows XP 32bit        -
 **   Tested in MS Windows 7  64bit        x
 **   Tested in Ubuntu10_10 Linux 32bit    -
 **   Tested in Ubuntu10_10 Linux 64bit    -
 **
 ** Workflow:
 **   Created:<data started>
 **   1st Vett: <programmer> on <date>
 **   2nd Vett: <peer> on <date>
 **   3rd Vett: <teamleader> on <date>
 **   4th Vett: Fortify Issues Fixed on <date>
 **   5th Vett: Tested in Harness on <date>
 **
 **
 ** I/0 Pairs:
 **   Good: 1st Set:io-TC_C_394_envGood1.xml
 **         2nd Set:io-TC_C_394_envGood2.xml
 **         3rd Set:io-TC_C_394_envGood3.xml
 **         4th Set:
 **         5th Set:
 **    Bad: 1st Set:io-TC_C_394_envBad1.xml
 *********************************************/



/*
 Base Test Program -- business.c

 Given two dates in the same year, this Base Test Program computes the number of
 business days between them.  A business day is described as any day which is not a
 weekend day or a holiday.  The dates that are holidays are provided via a file.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "interfile.h"

int numberDaysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

struct Date *holidays[20];
int numberOfHolidays = 0;

void *localMalloc(int size) {

	if (size <= 0) {
		printf("\nERROR: Nothing allocated\n");
		exit(1);
	}
	void *mem = malloc(size);
	if (mem == NULL) {
		printf("\nERROR: Nothing allocated\n");
		exit(1);
	}

	return mem;
}

// from http://en.wikipedia.org/wiki/Leap_year#Algorithm
//if year modulo 400 is 0
//       then is_leap_year
//else if year modulo 100 is 0
//       then not_leap_year
//else if year modulo 4 is 0
//       then is_leap_year
//else
//       not_leap_year

int isLeapYear(int year) {
	if ((year % 400) == 0) {
		return 1;
	}
	else if ((year % 100) == 0) {
		return 0;
	}
	else {
		return (year % 4) == 0;
	}
}

void copyDate(struct Date *x, struct Date *y) {
	x->year = y->year;
	x->month = y->month;
	x->day = y->day;
}

int dateEqual(struct Date *first, struct Date *second) {
	return first->year == second->year &&
		   first->month == second->month &&
		   first->day == second->day;
}

struct Date* newDate(int month, int day, int year) {
	struct Date *d = (struct Date *) localMalloc(sizeof(struct Date));
	d->month = month;
	d->day = day;
	d->year = year;

	return d;
}

int after(struct Date *first, struct Date *second) {
	if (first->year > second->year) {
		return 1;
	}
	else if (first->year == second->year) {
		if (first->month > second->month) {
			return 1;
		} else {
			if (first->month == second->month) {
				return first->day > second->day;
			}
		}
	}

	return 0;
}

int before(struct Date *first, struct Date *second) {
	return after(second, first);
}

int computeNumHolidays(struct Date *first, struct Date *second) {
	int numOfHolidays = 0;
	int i;

	for (i = 0; i != numberOfHolidays; i++) {
		if(i>=20||holidays[i]==NULL)
			continue;
		if (dateEqual(first, holidays[i]) || dateEqual(holidays[i], second)) {
			numOfHolidays++;
		}
		else if (before(first, holidays[i]) && before(holidays[i], second)) {
			numOfHolidays++;
		}
		else if (after(holidays[i], second)) {
			break;
		}
	}
	//printf("Number of holidays for %d: %d\n", first->month, numOfHolidays);

	return numOfHolidays;

}

int computeDayOfTheWeek(struct Date *d) {
	//  algorithm taken from from http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week#An_algorithm_to_calculate_the_day_of_the_week
	int monthsTable[12] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};
	int c = 2 * (3 - (d->year/100 % 4));
	int y = d->year % 100;
	int m;

	if (isLeapYear(d->year)) {
		monthsTable[0] = 6;
		monthsTable[1] = 2;
	}
	m = monthsTable[d->month-1];
	int step3 = y + y/4;
	int step5 = c + m + step3 + d->day;

	return step5 % 7;
}

int computeNumWeekendDays(struct Date *first, struct Date *second) {
	int numOfWeekendDays = 0;
	struct Date currentDate;
	int doWofFirstDate, doW;

	// month and year must be equal
	if (first->year != second->year || first->month != second->month) {
		printf("\nERROR: procedure only works when the months and years are the same\n");
		exit(1);
	}
	copyDate(&currentDate, first);
	doWofFirstDate = computeDayOfTheWeek(&currentDate);
	doW = doWofFirstDate;
	while (!after(&currentDate, second)) {
		if (doW == 0 || doW == 6) {
			//printf("%d/%d/%d is a weekend\n", currentDate.month, currentDate.day, currentDate.year);
			numOfWeekendDays++;
		}
		currentDate.day++;
		doW = ((doW + 1) % 7);
	}
	//printf("Number of weekend days for %d: %d\n", first->month, numOfWeekendDays);

	return numOfWeekendDays;
}

int computeNonWorkDays(struct Date *first, struct Date *second) {
	return computeNumHolidays(first, second) + computeNumWeekendDays(first, second);
}

int doActualWork(struct Date *first, struct Date *second) {
	int numberOfDaysSoFar = 0;
	int currentMonth;

	if (first->month == second->month) {
		return (second->day - first->day) - computeNonWorkDays(first, second) + 1; //
	}

	// switch the dates if they are in the "wrong" order
		if ((first->month > second->month) ||
			((first->month == second->month) &&
			 (first->day > second->day))) {
			struct Date temp;
			copyDate(&temp, first);
			copyDate(first, second);
			copyDate(second, &temp);
		}

	currentMonth = first->month;

	while (1) {
		if (currentMonth == second->month) {
			struct Date *temp = newDate(currentMonth, 1, first->year);
			numberOfDaysSoFar += second->day;
			numberOfDaysSoFar -= computeNonWorkDays(temp,
													second);
			if (temp != NULL) {
				free(temp);
			}
			return numberOfDaysSoFar;
		}
		else if (currentMonth == first->month) {
			// add 1 to be inclusive of the end date....
			struct Date *second = newDate(currentMonth,
									      numberDaysPerMonth[first->month-1],
										  first->year);
			numberOfDaysSoFar += numberDaysPerMonth[first->month-1] - first->day + 1;
			numberOfDaysSoFar -= computeNonWorkDays(first, second);
			if (second != NULL) {
				free(second);
			}
		}
		else {
			// include all days of the current month
			struct Date *tempStart = newDate(currentMonth, 1, first->year);
			struct Date *tempEnd = newDate(currentMonth,
					   	   	   	    numberDaysPerMonth[currentMonth-1],
					   	   	   	    first->year);
			numberOfDaysSoFar += numberDaysPerMonth[currentMonth-1];
			numberOfDaysSoFar -=
					computeNonWorkDays(tempStart, tempEnd);
			if (tempStart != NULL) {
				free(tempStart);
			}
			if (tempEnd != NULL) {
				free(tempEnd);
			}
		}

		currentMonth++;
	}
}

int indexOf(char* s, char c, int sLengthEstimate, int errorExit) {
	int index = 0;
	while (*(s+index) != '\0') {
		if (*(s+index) == c) {
			return index;
		}
		sLengthEstimate--;
		if (sLengthEstimate == 0) {
			printf("\nERROR: Estimated string length exceeded before finding %c", c);
			exit(1);
		}
		index++;
	}
	if (errorExit) {
		printf("\nERROR: Specified character not found in the target string.\n");
		exit(1);
	} else {
		return -1;
	}
}

char* substring(char *s, int begin, int end) {
	int index;
	int bufferIndex = 0;
	int length = strlen(s);
	char *buffer = (char*)localMalloc(length+1);

	for (index = 0; index < length; index++) {
		if (index >= begin && index < end) {
			buffer[bufferIndex] = s[index];
			bufferIndex++;
		}
	}
	buffer[bufferIndex] = '\0';

	return buffer;
}

int validateAndConvertMonth(char monthString[]) {
	int length = strlen(monthString);
	int month;

	if (length == 1) {
		if (!isdigit((unsigned char)monthString[0])) {
			printf("\nERROR: Non digit in month %s\n", monthString);
			exit(1);
		}
	}
	else if (length == 2) {
		if (!isdigit((unsigned char)monthString[0]) || !isdigit((unsigned char)monthString[1])) {
			printf("\nERROR: Non digit in month %s\n", monthString);
			exit(1);
		}
	}
	else {
		printf("\nERROR: month %s not legal\n", monthString);
		exit(1);
	}
	month = atoi(monthString);
	if (month < 1 || month > 12) {
		printf("\nERROR: month %d not legal\n", month);
		exit(1);
	}

	return month;
}

int validateAndConvertDay(char* dayString, int month, int year) {
	int length = strlen(dayString);
	int day;

	if (length == 1) {
		if (!isdigit((unsigned char)dayString[0])) {
			printf("\nERROR: Non digit in day %s\n", dayString);
			exit(1);
		}
	}
	else if (length == 2) {
		if (!isdigit((unsigned char)dayString[0]) || !isdigit((unsigned char)dayString[1])) {
			printf("\nERROR: Non digit in day %s\n", dayString);
			exit(1);
		}
	}
	else {
		printf("\nERROR: Day %s is more than two characters.\n", dayString);
		exit(1);
	}
	day = atoi(dayString);
	if (day >= 1 && day <= numberDaysPerMonth[month-1]) {
		return day;
	}
	else if (month == 2 && isLeapYear(year) && day == 29) {
		return day;
	} else {
		printf("\nERROR: Day %s is not valid with given month %d and year %d.\n", dayString, month, year);
		exit(1);
	}
}

struct Date* newDateFromString(char* dateAsString) {
	int dateAsStringLength = strlen(dateAsString);
	struct Date *d = (struct Date *) localMalloc(sizeof(struct Date));
	char* currentP = dateAsString;
	int yearLength, indexOfSlash;
	char* dayString;

	if (dateAsStringLength < 8 || dateAsStringLength > 10) {
		printf("\nERROR: Date %s is not the correct length, must be between 8 and 10 characters.\n", dateAsString);
		exit(1);
	}

	indexOfSlash = indexOf(currentP, '/', 10, 1);
	d->month = validateAndConvertMonth(substring(currentP, 0, indexOfSlash));
	currentP += indexOfSlash+1;
	indexOfSlash = indexOf(currentP, '/', 10, 1);
	dayString = substring(currentP, 0, indexOfSlash);
	currentP += indexOfSlash+1;
	yearLength = strlen(currentP);
	if (yearLength != 4 || !isdigit((unsigned char)currentP[0]) || !isdigit((unsigned char)currentP[1]) || !isdigit((unsigned char)currentP[2]) || !isdigit((unsigned char)currentP[3])) {
		printf("\nERROR: Year %s is not 4 digit characters\n", currentP);
		exit(1);
	}
	d->year = atoi(currentP);
	d->day = validateAndConvertDay(dayString, d->month, d->year);
	if (dayString != NULL) {
		free(dayString);
	}

	return d;
}

//unsigned long readHolidays(int year) {
//	char holidayFileName[22] = "testData/holiday.";
//	char yearAsString[5];
//	FILE *holidayf = NULL;
//	char line[80];
//	int i = 0;
//
//	if (year > 9999) {
//		//printf("\nERROR: Cannot handle more than 4 digit years");
//		return -1;
//	}
//	sprintf(yearAsString, "%4d", year);
//	yearAsString[4] = '\0';
//	strncat(holidayFileName, (const char*) yearAsString, 4);
//	holidayFileName[21] = '\0';
//	holidayf = fopen(holidayFileName, "r");
//	if (holidayf == NULL) {
//		//printf("\nERROR: No holiday file %s for year %d\n", holidayFileName, year);
//		return -1;
//	}
//
//	while (fgets(line, 80, holidayf) != NULL) {
//		// make it unix friendly
//		int placeForNull = indexOf(line, '\r', 80, 0);
//		if (placeForNull == -1) {
//			placeForNull = indexOf(line, '\n', 80, 1);
//		}
//		line[placeForNull] = '\0';
//		holidays[i++] = newDateFromString(line);
//	}
//	fclose(holidayf);
//
//	return i;
//}

int main(int argc, char** argv) {

	char *dates;

	dates = getenv("dates");//STONESOUP:INTERACTION_POINT//STONESOUP:environment_variable

	if (dates == NULL)
	{
		printf("\nERROR: Need two dates as arguments\n");
		exit(1);
	}


//	if (argc < 3) {
//		printf("\nERROR: Need two dates as arguments\n");
//		exit(1);
//	}

	char* firstDateString = strtok(dates, " ");
	char* secondDateString = strtok(NULL, " ");


	struct Date *firstDate = newDateFromString(firstDateString);
	struct Date *secondDate = newDateFromString(secondDateString);

	if (firstDate->year != secondDate->year) {
		printf("\nERROR: Dates must be in the same year\n");
		exit(1);
	}
	// switch the dates if they are in the "wrong" order
	if ((firstDate->month > secondDate->month) ||
		((firstDate->month == secondDate->month) &&
		 (firstDate->day > secondDate->day))) {
		struct Date temp;
		copyDate(&temp, firstDate);
		copyDate(firstDate, secondDate);
		copyDate(secondDate, &temp);
	}
	// Blame Pope Gregory XIII....
	if (firstDate->year < 1753) {
		printf("\nERROR: This software only works for dates of years after 1752\n");
		exit(1);
	}
	// add 1 to the number of days in Feburary if it is a leap year
	if (isLeapYear(firstDate->year)) {
		numberDaysPerMonth[1]++;  // add 1 for leap year
	}
	//numberOfHolidays = readHolidays(firstDate->year);
	numberOfHolidays = interfileFunction(firstDate->year);//STONESOUP:interfile_2

	printf("RESULT: answer is: %d\n", doActualWork(firstDate, secondDate));
	return 0;
}

/* End of file */
