#ifndef _GENERAL_UTILITY
#define _GENERAL_UTILITY

enum STR2NUM_ERROR { STR2_SUCCESS, STR2_OVERFLOW, STR2_UNDERFLOW, STR2_INCONVERTIBLE };
STR2NUM_ERROR str2int (int &i, char const *s, int base = 0);
STR2NUM_ERROR str2uint (unsigned int &i, char const *s, int base = 0);

#endif
