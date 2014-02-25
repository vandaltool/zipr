#include <string>
#include <map>
#include <iostream>
#include <fstream>

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

void initQueryStructureCache(const char *p_filename)
{
	string queryStructure;
	ifstream f(p_filename);
	if (f.is_open())
	{
		while (getline(f, queryStructure))
		{
			query_structure_cache[queryStructure] = 1;
		}

		f.close();
	}
}

void saveQueryStructureCache(const char *p_filename)
{
	ofstream f(p_filename);
	if (!f.is_open())
		return;
	std::map<std::string, int>::iterator i;
	for (i = query_structure_cache.begin(); i != query_structure_cache.end(); i++)
	{
		f << i->first << endl;
	}

	f.close();
}

}
