#include "spasm.h"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

///Utility SPASM's main
int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        cerr<<"SPASM Usage:\n<input file> <output file> \n"<<endl;
        exit(1);
    }

    string input, output;

    input = string(argv[1]);
    output = string(argv[2]);

    try
    {
        a2bspri(input,output);
    }
    catch (SpasmException err)
    {
        cerr<<err.what()<<endl;
        exit(1);
    }
}
