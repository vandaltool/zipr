#include <libIRDB-util.hpp>
#include <fcntl.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <transform_step.h>
#include <algorithm>
#include <assert.h>


using namespace std;
using namespace libIRDB;
using namespace Transform_SDK;


#define MAX_BUF 1024

enum class Mode { DEBUG, VERBOSE, DEFAULT };

int execute_step(int argc, char* argv[], bool step_optional, Mode exec_mode, 
                 IRDBObjects_t* shared_objects, TransformStep_t* the_step);


// The toolchain driver script ps_analyze.sh communicates
// with this program via two pipes. This allows DB objects to be held
// in memory and shared across steps without rewriting the entire ps_analyze.sh
// script in C++.
int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        cerr << "Usage: thanos.exe <input pipe name> <output pipe name>" << endl;
        return 1;
    }
    
    int in_pipe_fd, out_pipe_fd, num_bytes_read;
    char* input_pipe = argv[1];
    char* output_pipe = argv[2];
    char buf[MAX_BUF];
    buf[0] = '\0';

    const char* base_path = getenv("SECURITY_TRANSFORMS_HOME");
    if(base_path == NULL)
    {
	cerr << "Environment variables not set." << endl;
	return 1;
    }
    string plugin_path (string(base_path).append("/plugins_install/"));

    in_pipe_fd = open(input_pipe, O_RDONLY);
    if (in_pipe_fd == -1) {
        cerr << "Not a valid pipe name." << endl;
        return 1;
    }

    out_pipe_fd = open(output_pipe, O_WRONLY);
    if (out_pipe_fd == -1) {
        cerr << "Not a valid pipe name." << endl;
        return 1;
    }
   
    // Main loop where ps_analyze communicates with thanos.exe
    // to execute steps that conform to the Transform Step SDK.    
    Mode exec_mode = Mode::DEBUG;
    IRDBObjects_t* shared_objects = new IRDBObjects_t();    
    while (true) 
    {
        if((num_bytes_read = read(in_pipe_fd, buf, MAX_BUF)) > 0)
        {
            buf[num_bytes_read] = '\0';
            if(strncmp(buf, "SET_MODE", 8) == 0)
            {
                if(strcmp(buf+8, " DEBUG") == 0)
                {
                    exec_mode = Mode::DEBUG;
                }
                else if(strcmp(buf+8, " VERBOSE") == 0)
                {
                    exec_mode = Mode::VERBOSE;
                }
                else if(strcmp(buf+8, " DEFAULT") == 0)
                {
                    exec_mode = Mode::DEFAULT;
                }
                else
                {
                    ssize_t write_res = write(out_pipe_fd, (void*) "ERR_INVALID_CMD\n", 16);
                    if(write_res == -1)
                        return -1;
                }
            }
            else if(strncmp(buf, "EXECUTE_STEP", 12) == 0)
            {
                if(strncmp(buf+12, " OPTIONAL ", 10) == 0 || strncmp(buf+12, " CRITICAL ", 10) == 0)
                {
	 	    char *command_start = buf+12+10;
                    char *command_end = strchr(buf, '\n');
                    if (!command_end || (command_end == command_start))
                    {
                        ssize_t write_res = write(out_pipe_fd, (void*) "ERR_INVALID_CMD\n", 16);
                        if(write_res == -1)
                            return -1;
                    }
                    else
                    {
			// parse command string into argv, argc
			// simple parsing relies on ps_analyze.sh separating
			// step arguments with single spaces.
			*command_end = '\0';

			size_t max_args = ((command_end - command_start) / 2)+1;
                        char** argv = (char**) malloc(max_args);
			int argc = 0;
			
			argv[argc++] = command_start; 
			char* command_remaining = strchr(command_start, ' ');
                        while (command_remaining != NULL)
                        {
			    *command_remaining = '\0';
                            argv[argc++] = command_remaining+1;
			    command_remaining = strchr(command_remaining, ' ');
                        }
                        argv = (char**) realloc(argv, argc);

                        // load transform step plugin
			char* step_name = argv[0];                        
                        void* dlhdl = dlopen((plugin_path.append(step_name)).c_str(), RTLD_NOW);
                        if(dlhdl == NULL)
                        {
                            ssize_t write_res = write(out_pipe_fd, (void*) "STEP_UNSUPPORTED\n", 17);
                            if(write_res == -1)
                                return -1;
                        }
                        else
                  	{
                            void* sym = dlsym(dlhdl, "GetTransformStep"); 
                            if(sym == NULL)
                            {
			        ssize_t write_res = write(out_pipe_fd, (void*) "STEP_UNSUPPORTED\n", 17);
                                if(write_res == -1)
                                    return -1;
                            }
                            else
                            {
				TransformStep_t* (*func)(void);
				func = (TransformStep_t* (*)(void)) sym;
				TransformStep_t* the_step = (*func)();
				assert(the_step != NULL);

                                bool step_optional = true;
                                if(strncmp(buf+12, " CRITICAL ", 10) == 0)
                                {
                                    step_optional = false;
                                }
                                int step_retval = 0;

				step_retval = execute_step(argc, argv, step_optional, exec_mode, shared_objects, the_step);
                                delete the_step;
				free(argv);
                                dlclose(dlhdl);

				string step_retval_str(to_string(step_retval)+"\n");
                                ssize_t write_res = write(out_pipe_fd, (void*) step_retval_str.c_str(), step_retval_str.size()); // size() excludes terminating null character in this case, which is what we want
                                if(write_res == -1)
                                    return -1;
                            }
                        }
                    }
                }
                else
                {
                    ssize_t write_res = write(out_pipe_fd, (void*) "ERR_INVALID_CMD\n", 16);
                    if(write_res == -1)
                        return -1;
                }
            }
            else if(strcmp(buf, "TERMINATE") == 0)
            {
                // do thanos stuff
                break;
            }
            else
            {
                ssize_t write_res = write(out_pipe_fd, (void*) "ERR_INVALID_CMD\n", 16);
                if(write_res == -1)
                    return -1;
            }
        }
    }

    close(in_pipe_fd);
    close(out_pipe_fd);
    delete shared_objects;
	
    return 0;
}


int execute_step(int argc, char* argv[], bool step_optional, Mode exec_mode, 
                 IRDBObjects_t* shared_objects, TransformStep_t* the_step)
{
    int parse_retval = the_step->ParseArgs(argc, argv);
    if(parse_retval != 0)
    {
        return parse_retval;
    }
    
    pqxxDB_t* pqxx_interface = shared_objects->GetDBInterface();
    if(step_optional)
    {
        int error = shared_objects->WriteBackAll();
        if(error)
        {
            return -1; // the failure must be from a critical step, abort
        }
        else
        {
            // commit changes (in case this step fails) and reset interface
            pqxx_interface->Commit();
            pqxx_interface = shared_objects->ResetDBInterface();
        }
    }

    int step_error = the_step->ExecuteStep(shared_objects);

    if(step_error)
    {
        if(step_optional)
        {
            // delete all shared items without writing
            // next step will have to get the last "good" version from DB
            shared_objects->DeleteAll();
        }
        else
        {
            return -1; // critical step failed, abort
        }
    }
    
    if(step_optional)
    {
        // write changes to DB to see if it succeeds
        int error = shared_objects->WriteBackAll();
        if(error)
        {
            // abort changes by resetting DB interface
            pqxx_interface = shared_objects->ResetDBInterface();
        }
        else if(exec_mode == Mode::DEBUG)
        {
            // commit changes (in case next step fails) and reset interface 
            pqxx_interface->Commit();
            pqxx_interface = shared_objects->ResetDBInterface();
        }
    }
    else if(exec_mode == Mode::DEBUG)
    {
        // write changes to DB in case next step fails
        int error = shared_objects->WriteBackAll();
        if(error)
        {
            return -1; // critical step failed, abort
        }
        else
        {
            // commit changes (in case next step fails) and reset interface 
            pqxx_interface->Commit();
            pqxx_interface = shared_objects->ResetDBInterface();
        }
    }
    
    return step_error;
}
