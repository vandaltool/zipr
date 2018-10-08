#include <libIRDB-util.hpp>
#include <fcntl.h>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>


using namespace std;
using namespace libIRDB;


#define MAX_BUF 12


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
    while (strcmp(buf, "THANOS_DONE") != 0) {
        if((i = read(fd, buf, MAX_BUF)) > 0)
        {
            buf[i] = '\0';
            printf("Received: [%s]\n", buf);
	    res = write(outfd, (void*) "END\n", 5);
        }
    }

    close(fd);
    close(outfd);
	
    if(res)
        return 0;
    else
        return -1;
}
