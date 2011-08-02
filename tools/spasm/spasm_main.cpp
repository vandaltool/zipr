#include "spasm.h"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

///Utility SPASM's main
int main(int argc, char *argv[])
{
    if(argc != 4)
    {
        cerr<<"SPASM Usage:\n<input file> <output file> <symbol file> \n"<<endl;
        exit(1);
    }

    string input, output, elf;

    input = string(argv[1]);
    output = string(argv[2]);
    elf = string(argv[3]);

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
