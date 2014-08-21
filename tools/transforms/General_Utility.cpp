#include "General_Utility.hpp"
#include <limits.h>
#include <cstdlib>
#include <cerrno>

using namespace std;

STR2NUM_ERROR str2int (int &i, char const *s, int base)
{
	char *end;
	long  l;
	errno = 0;
	l = strtol(s, &end, base);
	if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) {
		return STR2_OVERFLOW;
	}
	if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
		return STR2_UNDERFLOW;
	}
	if (*s == '\0' || *end != '\0') {
		return STR2_INCONVERTIBLE;
	}
	i = l;
	return STR2_SUCCESS;
}

//TODO: what if the string represents a negative number? Currently
//the number will be translated into an unsigned int. I could make this
//and incovertible situation. 
STR2NUM_ERROR str2uint (unsigned int &i, char const *s, int base)
{
	char *end;
	unsigned long  l;
	errno = 0;
	l = strtoul(s, &end, base);
	if ((errno == ERANGE && l == ULONG_MAX) || l > UINT_MAX) {
		return STR2_OVERFLOW;
	}
	if (*s == '\0' || *end != '\0') {
		return STR2_INCONVERTIBLE;
	}
	i = l;
	return STR2_SUCCESS;
}
