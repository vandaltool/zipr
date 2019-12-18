#ifndef cmdstr_hpp 
#define cmdstr_hpp 

#include <spawn.h> 
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <iostream>
#include <string>
#include <vector>
#include <array>

using namespace std;

static inline pair<string,int> command_to_string( const string& command)
{
	auto ret=string();
	int exit_code=0;
	posix_spawn_file_actions_t action;


	const auto pipe_closer = function<void(int*)>([](int *p_pipefd) -> void
		{
			const auto pipe = *p_pipefd;
			close(pipe);
			delete p_pipefd;
		});
	using PipeFD_t = unique_ptr<int,decltype(pipe_closer)>;

	const auto pipe_opener = [&]() -> vector<PipeFD_t>
		{
			auto pipe_fds = array<int,2>();
			if(pipe( pipe_fds.data()))
			{
				const auto err_str = string(strerror(errno));
				throw runtime_error("Cannot open pipe: " + err_str);
			}
			auto ret = vector<PipeFD_t>();
//			PipeFD_t p(new int(pipe_fds[0]), pipe_closer);
//			ret.push_back(move(p));
			ret.push_back({new int(pipe_fds[0]), pipe_closer});
			ret.push_back({new int(pipe_fds[1]), pipe_closer});
			return ret;
		};

	auto cout_pipe_vec = pipe_opener();
	auto cerr_pipe_vec = pipe_opener();
	const auto cout_pipe = vector<int>{*(cout_pipe_vec[0]), *(cout_pipe_vec[1])};
	const auto cerr_pipe = vector<int>{*(cerr_pipe_vec[0]), *(cerr_pipe_vec[1])};

	posix_spawn_file_actions_init(&action);
	posix_spawn_file_actions_addclose(&action, cout_pipe[0]);
	posix_spawn_file_actions_addclose(&action, cerr_pipe[0]);
	posix_spawn_file_actions_adddup2(&action, cout_pipe[1], 1);
	posix_spawn_file_actions_adddup2(&action, cerr_pipe[1], 2);

	posix_spawn_file_actions_addclose(&action, cout_pipe[1]);
	posix_spawn_file_actions_addclose(&action, cerr_pipe[1]);

	vector<char> argsmem[] = {{'s', 'h', '\0'}, {'-', 'c', '\0'}}; // allows non-const access to literals
	char *args[] = {&argsmem[0][0], &argsmem[1][0],const_cast<char*>(command.c_str()),nullptr};

	pid_t pid;
	if(posix_spawnp(&pid, args[0], &action, NULL, args, environ) != 0)
		cout << "posix_spawnp failed with error: " << strerror(errno) << "\n";

  	cout_pipe_vec[1].reset();
	cerr_pipe_vec[1].reset(); // close child-side of pipes


	// Read from pipes
	string buffer(1024,' ');
	vector<pollfd> plist = { {cout_pipe[0],POLLIN}, {cerr_pipe[0],POLLIN} };
	for ( int rval; (rval=poll(&plist[0],plist.size(),/*timeout*/-1))>0; ) 
	{
		if ( plist[0].revents&POLLIN) 
		{
			const auto bytes_read = read(cout_pipe[0], &buffer[0], buffer.length());
			ret += buffer.substr(0, static_cast<size_t>(bytes_read));
		}
		else if ( plist[1].revents&POLLIN ) 
		{
			const auto bytes_read = read(cerr_pipe[0], &buffer[0], buffer.length());
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

static inline int command_to_stream(const string& command, ostream& stream)
{
        cout << "Issuing command: " << command << endl;
        const auto res = command_to_string(command);

        stream << res.first << endl;
        return res.second;
}


#endif
