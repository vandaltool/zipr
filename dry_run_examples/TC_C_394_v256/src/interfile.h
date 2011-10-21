struct Date {
	int month;
	int day;
	int year;
};

extern struct Date *holidays[20];

unsigned long interfileFunction(int);

unsigned long readHolidays(int);

struct Date* newDateFromString(char* dateAsString);

int indexOf(char* s, char c, int sLengthEstimate, int errorExit);

/* End of file */
