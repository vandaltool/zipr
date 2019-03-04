#include <stdlib.h>
#include <fstream>
#include <string>
#include "al.hpp"
#include <irdb-core>
#include <getopt.h>

using namespace std;
using namespace IRDB_SDK;
using namespace AddLib;

void usage(string programName)
{
	cerr << "Usage: " << programName << " <variant id> <annotation file>" 	<<endl;
	cout<<"--append lib			Append named library"		<<endl;
	cout<<"-a lib"								<<endl;
	cout<<""<<endl;
	cout<<"--prepend lib			Prepend named library"		<<endl;
	cout<<"-p lib"								<<endl;
	cout<<""<<endl;
	cout<<"--help,--usage,-?,-h		Display this message"<<endl;
}

int main(int argc, char **argv)
{	
	string programName(argv[0]);
	char *strtolError = NULL;
	int transformExitCode = 0;
	int variantID = -1; 
	auto pqxx_interface=pqxxDB_t::factory();
	int exit_code=0;

	/*
	 * Check that we've been called correctly:
	 * <program> <variant id> <annotation file>
	 */
	if(argc < 2)
	{
		usage(programName);
		exit(1);
	}
	variantID = strtol(argv[1], &strtolError, 10);
	if (*strtolError != '\0')
	{
		cerr << "Invalid variantID: " << argv[1] << endl;
		exit(1);
	}

	// Parse some options for the transform
	const static struct option long_options[] = {
		{"prepend", required_argument, 0, 'p'},
		{"append", required_argument, 0, 'a'},
		{"help", no_argument, 0, 'h'},
		{"usage", no_argument, 0, '?'},
		{0,0,0,0}
	};
	auto short_opts="p:ah?";

	auto prepended_libs=set<string>();
	auto appended_libs=set<string>();

	while(1) {
		int index = 0;
		int c = getopt_long(argc, argv,short_opts, long_options, &index);
		if(c == -1)
			break;
		switch(c) {
			case 0:
				break;
			case 'p':
				prepended_libs.insert(string(optarg));
				break;
			case 'a':
				appended_libs.insert(string(optarg));
				break;
				break;
			case '?':
			case 'h':
				usage(argv[0]);
				exit(1);
				break;
			default:
				break;
		}
	}


	/* setup the interface to the sql server */
	BaseObj_t::setInterface(pqxx_interface.get());

	auto pidp=VariantID_t::factory(variantID);
	assert(pidp && pidp->isRegistered()==true);

	for(set<File_t*>::iterator it=pidp->getFiles().begin();
	    it!=pidp->getFiles().end();
		++it)
	{
		try 
		{
			/* read the IR from the DB */
			File_t* this_file = *it;
			auto firp = FileIR_t::factory(pidp.get(), this_file);
			cout<<"Transforming "<<this_file->getURL()<<endl;
			assert(firp && pidp);


			/*
			 * Create a transformation and then
			 * invoke its execution.
			 */
				AddLib_t al(firp.get(),prepended_libs,appended_libs);
				transformExitCode = al.execute();
			/*
			 * If everything about the transformation
			 * went okay, then we will write the updated
			 * set of instructions to the database.
			 */
			if (transformExitCode == 0)
			{
				if(getenv("MG_NOUPDATE")==NULL)
				{
					firp->writeToDB();
				}
			}
			else
			{
				cerr << programName << ": transform failed. Check logs." << endl;
				exit_code=2;
			}
		}
		catch (DatabaseError_t pnide)
		{
			cerr << programName << ": Unexpected database error: " << pnide << endl;
			exit(1);
		}
		catch (const std::exception &exc)
		{
		    // catch anything thrown within try block that derives from std::exception
			std::cerr << "Unexpected exception: " << exc.what();
			exit(1);
		}
		catch (...)
		{
			cerr << programName << ": Unexpected error" << endl;
			exit(1);
		}
	}
	pqxx_interface->commit();

	return exit_code;
}
