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

int execute_step(int argc, char* argv[], bool step_optional, 
                 IRDBObjects_t* shared_objects, TransformStep_t* the_step);

// The toolchain driver scripts such as ps_analyze communicate
// with this program via a pipe. This allows DB objects to be held
// in memory and shared across steps without rewriting the entire ps_analyze
// script in C++.
int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        cerr << "Usage: thanos.exe <input pipe name> <output pipe name>" << endl;
    }
    
    int fd, i;
    char* input_pipe = argv[1];
    char* output_pipe = argv[2];
    char buf[MAX_BUF];
    buf[0] = '\0';

    const char* base_path = getenv("SECURITY_TRANSFORMS_HOME");
    if(base_path == NULL)
    {
	cerr << "Environment variables not set." << endl;
	return -1;
    }
    string plugin_path (string(base_path).append("/plugins_install/"));

    fd = open(input_pipe, O_RDONLY);
    if (fd == -1) {
        cerr << "Not a valid pipe name." << endl;
        return 1;
    }

    int outfd = open(output_pipe, O_WRONLY);
    if (outfd == -1) {
        cerr << "Not a valid pipe name." << endl;
        return 1;
    }
   
    ssize_t res = 1; 
    // Main loop where ps_analyze communicates with thanos.exe
    // to execute steps that conform to the Transform Step SDK.
    enum class Mode { DEBUG, VERBOSE, DEFAULT };
    
    Mode exec_mode = Mode::DEFAULT;
    IRDBObjects_t* shared_objects = new IRDBObjects_t();
    
    while (true) 
    {
        if((i = read(fd, buf, MAX_BUF)) > 0)
        {
            buf[i] = '\0';
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
                    res = write(outfd, (void*) "ERR_INVALID_CMD\n", 16);
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
                        res = write(outfd, (void*) "ERR_INVALID_CMD\n", 16);
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

			char* step_name = argv[0];                        
                        void* dlhdl = dlopen((plugin_path.append(step_name)).c_str(), RTLD_NOW);
                        if(dlhdl == NULL)
                        {
                            res = write(outfd, (void*) "STEP_UNSUPPORTED\n", 17);
                        }
                        else
                  	{
                            void* sym = dlsym(dlhdl, "GetTransformStep"); 
                            if(sym == NULL)
                            {
			        res = write(outfd, (void*) "STEP_UNSUPPORTED\n", 17);
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

				if(exec_mode == Mode::DEFAULT)
				    step_retval = execute_step(argc, argv, step_optional, shared_objects, the_step);
                                delete the_step;
				free(argv);
                                dlclose(dlhdl);
                                res = write(outfd, (void*) "STEP_RETVAL\n", 12);
                                assert(sizeof(step_retval) == 4);
                            }
                        }
                    }
                }
                else
                {
                    res = write(outfd, (void*) "ERR_INVALID_CMD\n", 16);
                }
            }
            else if(strcmp(buf, "TERMINATE") == 0)
            {
                // do thanos stuff
                break;
            }
            else
            {
                res = write(outfd, (void*) "ERR_INVALID_CMD\n", 16);
            }
        }
    }

    close(fd);
    close(outfd);
    delete shared_objects;
	
    if(res)
        return 0;
    else
        return -1;
}


int execute_step(int argc, char* argv[], bool step_optional, 
                 IRDBObjects_t* shared_objects, TransformStep_t* the_step)
{
    int parse_retval = the_step->ParseArgs(argc, argv);
    if(parse_retval != 0)
    {
        return parse_retval;
    }
    
    if(step_optional)
    {
        shared_objects->WriteBackAll();        
    }
    
    // FOR TESTING: print args
    for(int i = 0; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 15; // for test
}
