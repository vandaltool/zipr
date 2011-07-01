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
