#include <stdlib.h>
#include <fstream>
#include <irdb-core>

#include "mg.hpp"
#include <getopt.h>

using namespace std;
using namespace IRDB_SDK;

#define ALLOF(a) begin(a),end(a)

class MoveGlobalsDriver_t :  public TransformStep_t
{


void usage(string programName)
{

	auto et_names=string();
	for(const auto &str : elftable_names )
		et_names+=str+" ";

	cerr << "Usage: " << programName << " <variant id> <annotation file>" <<endl;
	cout<<"--elftables-only,-o		Move sections titled \""<<et_names<<"\""<<endl;
	cout<<"--aggressive 			Use aggressive heuristics to move more variables (does not affect elftables) "<<endl;
	cout<<"--no-conservative 		alias for --aggressive"<<endl;
	cout<<"--conservative 			(default) Use conservative heuristics to increase reliability (does not affect elftables) "<<endl;
	cout<<"--no-aggressive 			alias for --conservative"<<endl;

	cout<<endl;
	cout<<"---- debugging options (power users only) "<<endl;
	cout<<"--use-stars 			Enable STARS deep analysis for more analysis precision (current default). "<<endl;
	cout<<"--no-use-stars 			Disable STARS deep analysis for more analysis precision. "<<endl;
	cout<<"--move,-m			Move only the given objects."<<endl;
	cout<<"--dont,-d			Dont move the listed objects (overrides --only)."<<endl;
	cout<<"--number,-n			Max number of scoops to move."<<endl;
        cout<<"--random,-r                      Randomly select scoops to move."<<endl;
	cout<<"--help,--usage,-?,-h		Display this message"<<endl;
}

DatabaseID_t variantID = BaseObj_t::NOT_IN_DATABASE;
string dont_move = "";
string move_only = "";
size_t max_moveables = 0;
bool random = false;
bool aggressive = false;
VariantID_t* pidp = nullptr;
const string programName=string("libmove_globals.so");
bool use_stars=true;

int parseArgs(const vector<string> step_args) 
{	

        auto argv = vector<char*>();
        transform(ALLOF(step_args), back_inserter(argv), [](const string &s) -> char* { return const_cast<char*>(s.c_str()); } );
	const auto argc = step_args.size();
	auto strtolError = (char*) nullptr;

	/*
	 * Check that we've been called correctly:
	 * <program> <variant id> <annotation file>
	 */
	if(argc < 1)
	{
		usage(programName);
		return 2;
	}
	variantID = strtol(step_args[0].c_str(), &strtolError, 10);
	if (*strtolError != '\0')
	{
		cerr << "Invalid variantID: " << step_args[0] << endl;
		return 1;
	}

	// Parse some options for the transform
	const static struct option long_options[] = {
		{"elftables-only", no_argument, 0, 'o'},
		{"use-stars", no_argument, 0, 's'},
		{"no-use-stars", no_argument, 0, 't'},
		{"move", required_argument, 0, 'm'},
		{"dont", required_argument, 0, 'd'},
		{"number", required_argument, 0, 'n'},
		{"aggressive", no_argument, 0, 'a'},
		{"no-aggressive", no_argument, 0, 'A'},
		{"conservative", no_argument, 0, 'A'},
		{"no-conservative", no_argument, 0, 'a'},
                {"random", no_argument, 0, 'r'},
		{"help", no_argument, 0, 'h'},
		{"usage", no_argument, 0, '?'},
		{0,0,0,0}
	};
	auto short_opts="b:oh?m:d:n:aAst";
	while(1) 
	{
		int index = 0;
		int c = getopt_long(argc, &argv[0], short_opts, long_options, &index);
		if (c == -1)
			break;
		switch(c) {
			case 0:
				break;
			case 'c':
			case 'o':
				// add elftables to move only list
				for(const auto &str : elftable_names )
					move_only+= str+" ";
				break;
			case 's':
					use_stars=true;
				break;
			case 't':
					use_stars=false;
				break;
			case 'm':
				move_only+=string(" ") + optarg;
				break;
			case 'd':
				dont_move+=string(" ") + optarg;
				break;
			case 'n':
				max_moveables+=strtoll(optarg,nullptr,0);
				break;
                       case 'r':
                               random=true;
                               break;
			case 'a':
				cout<<"Setting aggressive mode"<<endl;
				aggressive=true;
				break;
			case 'A':
				cout<<"Setting conservative mode"<<endl;
				aggressive=false;
				break;
			case '?':
			case 'h':
				usage("libmove_globals.so");
				return 1;
			default:
				break;
		}
	}
	return 0;
}


int executeStep(IRDBObjects_t *const irdb_objects)
{

	auto exit_code = (int) 0;

	/* setup the interface to the sql server */
	const auto pqxx_interface=irdb_objects->getDBInterface();
	BaseObj_t::setInterface(pqxx_interface);

	// get the variant info from the database
	pidp=irdb_objects->addVariant(variantID); // new VariantID_t(variantID);
	assert(pidp && pidp->isRegistered()==true);
	auto transformExitCode = (int) 0;

	for(auto this_file : pidp->getFiles())
	{
		try 
		{
			/* read the IR from the DB */
			auto firp =  irdb_objects->addFileIR(variantID, this_file->getBaseID()); // new FileIR_t(*pidp, this_file);
			cout<<"Transforming "<<this_file->getURL()<<endl;
			assert(firp && pidp);


			/*
			 * Create a transformation and then
			 * invoke its execution.
			 */
			if (firp->getArchitectureBitWidth() == 64)
			{	
                		MoveGlobals64_t mg(pidp, firp, dont_move, move_only, max_moveables, random, aggressive, use_stars);
				transformExitCode = mg.execute(*pqxx_interface);
			}
			else
			{
                		MoveGlobals32_t mg(pidp, firp, dont_move, move_only, max_moveables, random, aggressive, use_stars);
				transformExitCode = mg.execute(*pqxx_interface);
			}
			/*
			 * If everything about the transformation
			 * went okay, then we will write the updated
			 * set of instructions to the database.
			 */
			if (transformExitCode != 0)
			{
				cerr << programName << ": transform failed. Check logs." << endl;
				exit_code=2;
			}
		}
		catch (DatabaseError_t pnide)
		{
			cerr << programName << ": Unexpected database error: " << pnide << endl;
			return 1;
		}
		catch (const std::exception &exc)
		{
		    // catch anything thrown within try block that derives from std::exception
			std::cerr << "Unexpected exception: " << exc.what();
			return 1;
		}
		catch (...)
		{
			cerr << programName << ": Unexpected error" << endl;
			return 1;
		}
	}

	return exit_code;
}

std::string getStepName(void) const override
{
        return std::string("move_globals");
}

};

extern "C"
shared_ptr<TransformStep_t> getTransformStep(void)
{
        return shared_ptr<TransformStep_t>(new MoveGlobalsDriver_t());
}

