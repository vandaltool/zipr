/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

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
        return s2n_OVERFLOW;
    }
    if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
        return s2n_UNDERFLOW;
    }
    if (*s == '\0' || *end != '\0') {
        return s2n_INCONVERTIBLE;
    }
    i = l;
    return s2n_SUCCESS;
}

//TODO: what if the string represents a negative number? Currently
//the number will be translated into an unsigned int. I could make this
//and incovertible situation. 
STR2NUM_ERROR str2uint (unsigned int &i, char const *s, int base)
{
    char *end;
    unsigned long  l;
    errno = 0;
    l = strtol(s, &end, base);
    if ((errno == ERANGE && l == ULONG_MAX) || l > UINT_MAX) {
        return s2n_OVERFLOW;
    }
    if (*s == '\0' || *end != '\0') {
        return s2n_INCONVERTIBLE;
    }
    i = l;
    return s2n_SUCCESS;
}

void trim(string& str)
{
    string::size_type pos = str.find_last_not_of(" \t\f\v\n\r");
    if(pos != string::npos) 
    {
    	str.erase(pos + 1);
        pos = str.find_first_not_of(" \t\f\v\n\r");
        if(pos != string::npos) str.erase(0, pos);
    }
    else 
        str.erase(str.begin(), str.end());
}


void tokenize(vector<string>& tokens, const string& str,const string& delimiters)
{
    tokens.clear();
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));

        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

