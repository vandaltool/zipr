#ifndef cmdstr_hpp 
#define cmdstr_hpp 

/*BINFMTCXX: -std=c++11 -Wall -Werror
*/

#include <spawn.h> // see manpages-posix-dev
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

static inline pair<string,int> command_to_string( const string& command)
{
	auto ret=string();
	int exit_code;
	int cout_pipe[2];
	int cerr_pipe[2];
	posix_spawn_file_actions_t action;

	if(pipe(cout_pipe) || pipe(cerr_pipe))
		cout << "pipe returned an error.\n";

	posix_spawn_file_actions_init(&action);
	posix_spawn_file_actions_addclose(&action, cout_pipe[0]);
	posix_spawn_file_actions_addclose(&action, cerr_pipe[0]);
	posix_spawn_file_actions_adddup2(&action, cout_pipe[1], 1);
	posix_spawn_file_actions_adddup2(&action, cerr_pipe[1], 2);

	posix_spawn_file_actions_addclose(&action, cout_pipe[1]);
	posix_spawn_file_actions_addclose(&action, cerr_pipe[1]);

	//  string command = "echo bla"; // example #1
	//  string command = "pgmcrater -width 64 -height 9 |pgmtopbm |pnmtoplainpnm";
	vector<char> argsmem[] = {{'s', 'h', '\0'}, {'-', 'c', '\0'}}; // allows non-const access to literals
	char *args[] = {&argsmem[0][0], &argsmem[1][0],const_cast<char*>(command.c_str()),nullptr};

	pid_t pid;
	if(posix_spawnp(&pid, args[0], &action, NULL, args, environ) != 0)
		cout << "posix_spawnp failed with error: " << strerror(errno) << "\n";

	close(cout_pipe[1]), close(cerr_pipe[1]); // close child-side of pipes

	// Read from pipes
	string buffer(1024,' ');
	std::vector<pollfd> plist = { {cout_pipe[0],POLLIN}, {cerr_pipe[0],POLLIN} };
	for ( int rval; (rval=poll(&plist[0],plist.size(),/*timeout*/-1))>0; ) 
	{
		if ( plist[0].revents&POLLIN) {
			const auto bytes_read = read(cout_pipe[0], &buffer[0], buffer.length());
			cout << "read " << bytes_read << " bytes from stdout.\n";
			cout << buffer.substr(0, static_cast<size_t>(bytes_read)) << "\n";
			ret += buffer.substr(0, static_cast<size_t>(bytes_read));
		}
		else if ( plist[1].revents&POLLIN ) {
			const auto bytes_read = read(cerr_pipe[0], &buffer[0], buffer.length());
			cout << "read " << bytes_read << " bytes from stderr.\n";
			cout << buffer.substr(0, static_cast<size_t>(bytes_read)) << "\n";
			ret +=  buffer.substr(0, static_cast<size_t>(bytes_read));
		}
		else 
			break; // nothing left to read
	}

	waitpid(pid,&exit_code,0);
	cout << "exit code: " << exit_code << "\n";

	posix_spawn_file_actions_destroy(&action);
	return {ret,exit_code};
}

#endif
