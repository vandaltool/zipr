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
                    char *end = strchr(buf, '\n');
                    if (!end || (end == buf+22))
                    {
                        res = write(outfd, (void*) "ERR_INVALID_CMD\n", 16);
                    }
                    else
                    {
                        *end = '\0';
                        string command (buf+22);
                        
                        size_t step_path_end = command.find_first_of(" ");
                        char* step_path;
                        if(step_path_end == string::npos)
                        {
                            step_path = (buf+22);
                        }
                        else
                        {
                            step_path = (char*) (command.substr(0, step_path_end)).c_str();
                        }
                        void* dlhdl = dlopen(step_path, RTLD_LAZY);
                        if(dlhdl == NULL)
                        {
                            res = write(outfd, (void*) "STEP_UNSUPPORTED\n", 17);
                        }
                        else
                        {
			    TransformStep_t* (*func)(void);
                            func = (TransformStep_t* (*)(void)) dlsym(dlhdl, "TransformStepFactory");
                            if(func == NULL)
                            {
			        res = write(outfd, (void*) "STEP_UNSUPPORTED\n", 17);
                            }
                            else
                            {
	                        int argc = (int) count(command.begin(), command.end(), ' ')+1;
                                char** argv = (char**) malloc(argc);
                                argv[0] = buf+22;
                                size_t pos = 0;
                                int arg_num = 1;
                                while ((pos = command.find_first_of(" ", pos)) != string::npos)
                                {
                                    *(buf+22+pos) = '\0';
                                    argv[arg_num] = buf+22+pos+1;
                                    ++arg_num;
                                    ++pos;
                                }
                                assert(arg_num == argc);
                                bool step_optional = true;
                                if(strncmp(buf+12, " CRITICAL ", 10) == 0)
                                {
                                    step_optional = false;
                                }
				cout << "GOT HERE 3" << endl;
                                int step_retval = 0;
				if(exec_mode == Mode::DEFAULT)
				    step_retval = execute_step(argc, argv, step_optional, shared_objects, (*func)());
                                free(argv);
                                dlclose(dlhdl);
                                res = write(outfd, (void*) "STEP_RETVAL\n", 14);
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
        printf("%s ", argv[argc]);
    }
    printf("\n");
    return 15; // for test
}
