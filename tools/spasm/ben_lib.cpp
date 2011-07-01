#include "ben_lib.h"

using namespace std;

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


