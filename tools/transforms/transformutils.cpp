#include <iostream>
#include <fstream>
#include <string>

#include "transformutils.h"

using namespace std;

set<string> getFunctionList(char *p_filename)
{
	set<string> functionList;

	ifstream candidateFile;
	candidateFile.open(p_filename);

	if(candidateFile.is_open())
	{
		while(!candidateFile.eof())
		{
			string functionName;
			getline(candidateFile, functionName);

			functionList.insert(functionName);
		}

		candidateFile.close();
	}

	return functionList;
}

