#include "spasm.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

bool fexists(string filename)
{
  ifstream ifile(filename.c_str());
  return ifile;
}

///Utility SPASM's main
int main(int argc, char *argv[])
{
    string input, output, elf;

    if(argc == 4)
    {
    	elf = string(argv[3]);
	if(!fexists(elf))
	{
		cerr<<"Symbol file "<<elf<<" does not exist.  SPASM will not be able to process callbacks properly."<<endl;
	}
    }
    else if(argc == 5) 
    {
    	elf = string(argv[3]);
	if(!fexists(elf))
	{
    		elf = string(argv[4]);
		if(!fexists(elf))
			cerr<<"Symbol files ("<<argv[3] << " and " << argv[4] << 
				") do not exist.  SPASM will not be able to process callbacks properly."<<endl;
	}
		
    }
    else 
    {
        cerr<<"SPASM Usage:\n<input file> <output file> <symbol file> \n"<<endl;
        exit(1);
    }


    input = string(argv[1]);
    output = string(argv[2]);

	cout<<"Input:"<<input<<endl;
	cout<<"Output:"<<output<<endl;
	cout<<"Symbols:"<<elf<<endl;

    try
    {
        a2bspri(input,output,elf);
    }
    catch (SpasmException err)
    {
        cerr<<err.what()<<endl;
        exit(1);
    }
}
