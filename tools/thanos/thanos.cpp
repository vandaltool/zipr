#include <libIRDB-core.hpp>
#include <dlfcn.h> 
#include <vector>
#include <memory>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

using namespace std;
using namespace libIRDB;
using namespace Transform_SDK;

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
	// get plugins
	auto thanos_plugins = getPlugins(argc-1, argv+1);	
	if(thanos_plugins.size() == 0)
	{
		// for now, usage is pretty strict to enable simple
		// parsing, because this program is only used by an
		// automated script
		cout << "Syntax error in arguments." << endl;
		cout << "USAGE: (\"<step name> [-optional] [--step-args [ARGS]]\")+" << endl;
		return 1;
	}

	for(unsigned int i = 0; i < thanos_plugins.size(); ++i)
	{
		ThanosPlugin_t* plugin = thanos_plugins[i].get();

		const int result = plugin->runPlugin();
		// if that returns failure AND the step is not optional
		if(result != 0 && !plugin->isOptional())
		{
			cout << "A critical step failed: " << plugin->getStepName() << endl;
			cout << "If DEBUG_STEPS is not on, this failure could "
			     << "be due to an earlier critical step." << endl;	 
			return 1; // critical step failed, abort
		}
	}
	// write back final changes
	const int result = ThanosPlugin_t::saveChanges();
	if(result != 0)
	{
		cout << "A critical step failed: " << (thanos_plugins.back())->getStepName() 
		     << endl;
                cout << "If DEBUG_STEPS is not on, this failure could "
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
		cout << "Environment variables not set." << endl;
		return -1;
    	}
    	static const auto plugin_path (string(base_path).append("/plugins_install/"));

	void *const dlhdl = dlopen((plugin_path+"lib"+step_name+".so").c_str(), RTLD_NOW);
        if(dlhdl == NULL)
        {
        	const auto err=dlerror();
                cout<<"Cannot open "<<step_name<<": "<<err<<endl;
		return -1;
        }
        
       	const void *const sym = dlsym(dlhdl, "GetTransformStep"); 
        if(sym == NULL)
        {
        	const auto err=dlerror();
                cout<<"Cannot find GetTransformStep in "<<step_name<<": "<<err<<endl;
		return -1;
        }

	using GetTransformPtr_t = shared_ptr<TransformStep_t> (*)(void);  // function pointer, takes void, returns TransformStep_t shared ptr
	GetTransformPtr_t func=(GetTransformPtr_t)sym;
	shared_ptr<TransformStep_t> the_step = (*func)();
	assert(the_step != NULL);
	
	static const char *const are_debugging = getenv("DEBUG_STEPS");

	int saved_stdout = dup(STDOUT_FILENO);
        int saved_stderr = dup(STDERR_FILENO);  
        FILE *log_output = NULL; 

	bool are_logging = !((bool) are_debugging);
	if(are_logging)
	{
		// setup logging
		auto logfile_path = "./logs/"+step_name+".log";
		log_output = fopen(logfile_path.c_str(), "a");
		auto log_output_fd = fileno(log_output);
                dup2(log_output_fd, STDOUT_FILENO);
                dup2(log_output_fd, STDERR_FILENO); 
	}
	
	const int step_result = executeStep(*(the_step.get()), (bool) are_debugging);

	if(are_logging)
	{
		fclose(log_output);
	}
	dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);

	the_step.reset(); // explicitly get rid of the handle to the library so we can close it.
	dlclose(dlhdl);

	// return status of execute method
	return step_result; 
}


int ThanosPlugin_t::executeStep(TransformStep_t& the_step, const bool are_debugging)
{
    const int parse_retval = the_step.parseArgs(step_args);
    if(parse_retval != 0)
    {
        return parse_retval;
    }
    
    pqxxDB_t* pqxx_interface = shared_objects->getDBInterface();
    if(step_optional)
    {
        const int error = shared_objects->writeBackAll();
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
            // delete all shared items without writing
            // next step will have to get the last "good" version from DB
            shared_objects->deleteAll();
        }
        else
        {
            return 1; // critical step failed, abort
        }
    }
    
    if(step_optional)
    {
        // write changes to DB to see if it succeeds
        const int error = shared_objects->writeBackAll();
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
        const int error = shared_objects->writeBackAll();
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
        const int error = shared_objects->writeBackAll();
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

