#include <libIRDB-core.hpp>
#include <dlfcn.h> 
#include <vector>
#include <memory>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <ctime>


using namespace std;
using namespace libIRDB;
using namespace Transform_SDK;

#define ALLOF(a) begin(a),end(a)

// global to be used like cout/cerr for writing to the logs
ofstream thanos_log;
ostream *real_cout;
ostream *real_cerr;
string thanos_path;
bool redirect_opt=true;

class ThanosPlugin_t
{
    public:
        static unique_ptr<ThanosPlugin_t> pluginFactory(const string plugin_details);
	static int saveChanges();
	bool isOptional()
	{
		return step_optional;
	}
	string getStepName()
	{
		return step_name;
	}
	int runPlugin();

    private:
        // methods
        ThanosPlugin_t(const string p_step_name,
                       const bool p_step_optional,
                       const vector<string> p_step_args
                       )
                       :
                           step_name(p_step_name),
                           step_optional(p_step_optional),
                           step_args(p_step_args)
                       {
                       }
	int executeStep(TransformStep_t& the_step, const bool are_debugging);
	int commitAll();

        // data
        const string step_name;
        const bool step_optional;
        const vector<string> step_args;
	static const unique_ptr<IRDBObjects_t> shared_objects;
};
// initialize private static data member
const unique_ptr<IRDBObjects_t> ThanosPlugin_t::shared_objects(new IRDBObjects_t());

using PluginList_t = vector<unique_ptr<ThanosPlugin_t>>;
PluginList_t getPlugins(const int argc, char const *const argv[]); 

int main(int argc, char* argv[])
{
	thanos_path=argv[0];
	ostream my_real_cerr(cerr.rdbuf());
	ostream my_real_cout(cout.rdbuf());
        real_cerr=&my_real_cerr;
        real_cout=&my_real_cout;

 	thanos_log.open("logs/thanos.log", ofstream::out);

	if(!thanos_log)
	{
		cerr<<"Cannot open logs/thanos.log"<<endl;
		exit(1);
	}
	// get plugins
	auto argv_iter=1;
	while (true)
	{
		if(argv_iter >= argc)
		{
			break;
		}
		if(string(argv[argv_iter])=="--no-redirect")
		{
			redirect_opt=false;
			argv_iter++;
		}
		else
			break;
	}
	const auto plugin_argv_iter=argv_iter;

	auto thanos_plugins = getPlugins(argc-plugin_argv_iter, argv+plugin_argv_iter);	
	if(thanos_plugins.size() == 0)
	{
		// for now, usage is pretty strict to enable simple
		// parsing, because this program is only used by an
		// automated script
		thanos_log << "Syntax error in arguments." << endl;
		thanos_log << "USAGE: <thanos opts> (\"<step name> [-optional] [--step-args [ARGS]]\")+" << endl;
		return 1;
	}

	for(unsigned int i = 0; i < thanos_plugins.size(); ++i)
	{
		ThanosPlugin_t* plugin = thanos_plugins[i].get();

		const int result = plugin->runPlugin();
		// if that returns failure AND the step is not optional
		if(result != 0 && !plugin->isOptional())
		{
			thanos_log << "A critical step failed: " << plugin->getStepName() << endl;
			thanos_log << "If DEBUG_STEPS is not on, this failure could "
			     << "be due to an earlier critical step." << endl;	 
			return 1; // critical step failed, abort
		}
	}
	// write back final changes
	const int result = ThanosPlugin_t::saveChanges();
	if(result != 0)
	{
		thanos_log << "A critical step failed: " << (thanos_plugins.back())->getStepName() 
		     << endl;
                thanos_log << "If DEBUG_STEPS is not on, this failure could "
                     << "be due to an earlier critical step." << endl;
                return 1; // critical step failed, abort
	}
	else
	{
		return 0; // success :)
	}
}


PluginList_t getPlugins(const int argc, char const *const argv[])
{	
	PluginList_t plugins;
	
	for(auto i = 0; i < argc; ++i)
	{
		auto the_plugin = ThanosPlugin_t::pluginFactory(string(argv[i]));	
		if(the_plugin == nullptr)
			return PluginList_t();
		plugins.push_back(move(the_plugin));
	}
	return plugins;
}


// assumes that tokens are always space-separated
// (cannot be delineated by quotes, for example)
const vector<string> getTokens(const string arg_string)
{
	vector<string> tokens;
	istringstream arg_stream(arg_string);
    	string token; 
    	while (getline(arg_stream, token, ' ')) 
	{
               	tokens.push_back(token);
    	}
	return tokens;
}


unique_ptr<ThanosPlugin_t> ThanosPlugin_t::pluginFactory(const string plugin_details)
{
	auto tokens = getTokens(plugin_details);
	if(tokens.size() < 1)
		return unique_ptr<ThanosPlugin_t>(nullptr);

	const auto step_name = tokens[0];
	auto step_optional = false;
	vector<string> step_args;
	for(unsigned int i = 1; i < tokens.size(); ++i)
	{
		if(tokens[i] == "--step-args")
		{
			if(tokens.begin()+i+1 < tokens.end())
				step_args.assign(tokens.begin()+i+1, tokens.end());
			break;
		}
		else if(tokens[i] == "-optional")
		{
			step_optional = true;
		}
		else
		{
			return unique_ptr<ThanosPlugin_t>(nullptr);
		}
	}	
	return unique_ptr<ThanosPlugin_t>(new ThanosPlugin_t(step_name, step_optional, step_args));	
}


int ThanosPlugin_t::runPlugin()
{
	static const char *const base_path = getenv("SECURITY_TRANSFORMS_HOME");
        if(base_path == NULL)
        {
		thanos_log << "Environment variables not set." << endl;
		return -1;
    	}
    	static const auto plugin_path (string(base_path).append("/plugins_install/"));

	void *const dlhdl = dlopen((plugin_path+"lib"+step_name+".so").c_str(), RTLD_NOW);
        if(dlhdl == NULL)
        {
        	const auto err=dlerror();
                thanos_log<<"Cannot open "<<step_name<<": "<<err<<endl;
		return -1;
        }
        
       	const void *const sym = dlsym(dlhdl, "GetTransformStep"); 
        if(sym == NULL)
        {
        	const auto err=dlerror();
                thanos_log<<"Cannot find GetTransformStep in "<<step_name<<": "<<err<<endl;
		return -1;
        }

	using GetTransformPtr_t = shared_ptr<TransformStep_t> (*)(void);  // function pointer, takes void, returns TransformStep_t shared ptr
	GetTransformPtr_t func=(GetTransformPtr_t)sym;
	shared_ptr<TransformStep_t> the_step = (*func)();
	assert(the_step != NULL);
	
	static const char *const are_debugging = getenv("DEBUG_STEPS");


	auto saved_cerrbuf = cerr.rdbuf();
        auto saved_coutbuf = cout.rdbuf();
	ofstream logfile;

	auto are_logging = !((bool) are_debugging);
	if(are_logging)
	{
		// setup logging
		auto logfile_path = "./logs/"+step_name+".log";
		logfile.open(logfile_path,ofstream::out);
		if(!logfile)
		{
			*real_cout<<"Cannot open log file "<<logfile_path<<endl;
			exit(1);
		}
		if(redirect_opt)
		{
			cout.rdbuf(logfile.rdbuf());
			cerr.rdbuf(logfile.rdbuf());
		}
	}
	

	const auto start_time = clock();
  	const auto start_t=time(nullptr);
	const auto start_time_str = ctime(&start_t);

	const auto step_result = executeStep(*(the_step.get()), (bool) are_debugging);

        const auto end_time = clock();
  	const auto end_t=time(nullptr);
	const auto end_time_str = ctime(&end_t);
        const auto elapsed_time = (double)(end_time-start_time)/ CLOCKS_PER_SEC;

        cout<< "#ATTRIBUTE start_time=" << start_time_str ; // endl in time_str
        cout<< "#ATTRIBUTE end_time="   << end_time_str   ; // endl in time_str
        cout<< "#ATTRIBUTE elapsed_time="  << elapsed_time<<endl;
        cout<< "#ATTRIBUTE step_name=" << step_name<<endl;
        cout<< "#ATTRIBUTE step_command=  " << thanos_path << " " << step_name 
	    << " --step-args "; copy(ALLOF(step_args), ostream_iterator<string>(cout, " ")); cout<<endl;
        cout<< "#ATTRIBUTE step_exitcode="<<dec<<step_result<<endl;


	cerr.rdbuf(saved_cerrbuf);
	cout.rdbuf(saved_coutbuf);

	the_step.reset(); // explicitly get rid of the handle to the library so we can close it.
	dlclose(dlhdl);

	// return status of execute method
	return step_result; 
}


int ThanosPlugin_t::executeStep(TransformStep_t& the_step, const bool are_debugging)
{

	*real_cout<<"Performing step "<<the_step.getStepName()<< " [dependencies=unknown] ..."; // no endl intentionally.
	flush(*real_cout);


	const int parse_retval = the_step.parseArgs(step_args);
	if(parse_retval != 0)
	{
		*real_cout<<"Done.  Command failed! ***************************************"<<endl;
		if(!step_optional)
		{
			*real_cout<<"ERROR: The "<<the_step.getStepName()<<" step is necessary, but failed.  Exiting early."<<endl;	
		}
		return parse_retval;
	}

	pqxxDB_t* pqxx_interface = shared_objects->getDBInterface();
	if(step_optional)
	{
		const int error = shared_objects->writeBackAll(&thanos_log);
		if(error)
		{
			return 1; // the failure must be from a critical step, abort
		}
		else
		{
		    // commit changes (in case this step fails) and reset interface
		    pqxx_interface->Commit();
		    pqxx_interface = shared_objects->resetDBInterface();
		}
	}

	const int step_error = the_step.executeStep(shared_objects.get());

	if(step_error)
	{
		if(step_optional)
		{

                        *real_cout<<"Done.  Command failed! ***************************************"<<endl;
			// delete all shared items without writing
			// next step will have to get the last "good" version from DB
			shared_objects->deleteAll();
		}
		else
		{
                        *real_cout<<"Done.  Command failed! ***************************************"<<endl;
                        *real_cout<<"ERROR: The "<<the_step.getStepName()<<" step is necessary, but failed.  Exiting early."<<endl;
			return 1; // critical step failed, abort
		}
	}
	else
	{
			*real_cout<<"Done.  Successful."<<endl;
	}

	if(step_optional)
	{
		// write changes to DB to see if it succeeds
		const int error = shared_objects->writeBackAll(&thanos_log);
		if(error)
		{
			// abort changes by resetting DB interface
			pqxx_interface = shared_objects->resetDBInterface();
		}
		else if(are_debugging)
		{
			// commit changes (in case next step fails) and reset interface 
			pqxx_interface->Commit();
			pqxx_interface = shared_objects->resetDBInterface();
		}
	}
	else if(are_debugging)
	{
		// write changes to DB in case next step fails
		const int error = shared_objects->writeBackAll(&thanos_log);
		if(error)
		{
			return 1; // critical step failed, abort
		}
		else
		{
			// commit changes (in case next step fails) and reset interface 
			pqxx_interface->Commit();
			pqxx_interface = shared_objects->resetDBInterface();
		}
	}

	return step_error;	
}


int ThanosPlugin_t::saveChanges()
{
	pqxxDB_t* pqxx_interface = shared_objects->getDBInterface();
        const int error = shared_objects->writeBackAll(&thanos_log);
        if(error)
        {
        	return 1; // critical step failed, abort
        }
        else
        {
        	// commit changes and reset interface 
        	pqxx_interface->Commit();
        	pqxx_interface = shared_objects->resetDBInterface();
       		return 0;
        }
}

