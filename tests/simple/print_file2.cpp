#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char** argv)
{
    if(argc != 1)
    {
	ofstream myfile;
	myfile.open("Hello_World.txt");
	myfile<<"Hello World"<<endl;
	myfile.close();
    }
    else
    {
	ofstream myfile;
	myfile.open("So Long.txt");
	myfile<<"Bye Bye"<<endl;
	myfile.close();
	ofstream myfile2;
	myfile2.open("So Long2.txt");
	myfile2<<"Bye Bye2"<<endl;
	myfile2.close();
    }
}
