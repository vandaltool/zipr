#include <string>
#include <map>

extern "C" {
#include "sql_structure.h"
}

using namespace std;

static map<string, int> query_structure_cache;  

extern "C" {
void addQueryStructure(char *p_queryStructure)
{
	query_structure_cache[string(p_queryStructure)] = 1;
}

int findQueryStructure(char *p_queryStructure)
{
	return query_structure_cache.count(string(p_queryStructure));
}

}
