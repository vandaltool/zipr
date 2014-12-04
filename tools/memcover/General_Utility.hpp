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

#ifndef _GENERAL_UTILITY
#define _GENERAL_UTILITY

#include <string>
#include <vector>
#include <map>

enum STR2NUM_ERROR { SUCCESS, OVERFLOW, UNDERFLOW, INCONVERTIBLE };
STR2NUM_ERROR str2int (int &i, char const *s, int base = 0);
STR2NUM_ERROR str2uint (unsigned int &i, char const *s, int base = 0);
void trim(std::string &str);
void tokenize(std::vector<std::string> &tokens, const std::string &str,const std::string& delimiters=" \t\n\r");

template <class k, class v>
void getKeys(const std::map<k,v> &m, std::vector<k> &keys)
{
  
    for(typename std::map<k,v>::const_iterator it = m.begin(); it !=m.end(); ++it)
    {
        keys.push_back(it->first);
    }
} 

#endif
