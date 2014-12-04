/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#ifndef BENLIB
#define BENLIB
#include <vector>
#include <map>
#include <string>

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
