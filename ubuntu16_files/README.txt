
Steps to installing.

	0) Unpack the tarball (likely called zipr_toolchain.tgz) -- you must have done this already 
		to be reading this!
	1) Switch to the directory created during unpacking. i.e., cd zipr_toolchain
	2) Run "source set_env_vars"
	3) Run "./ubuntu16_files/install.sh" in this directory -- lots of packages 
		will be installed.
	4) Copy your ida.key for IdaPro7.0 to ./idaproCur/ida.key.  Alternately, if you have a UVA CS 
		account, you can use the UVA-CS dependability group's IDA server.  To use this, run
		"source set_ida_server".
	5) Run "postgres_setup.sh"
		This step sets up postgres to hold the Zipr database.  If you already 
		have postgres configured, it may damage your data.  Use with appropriate caution.
	6) Test  the installation:
		run "cd /tmp; $PSZ /bin/ls ./ls.ziprd"
		You should see the Zipr toolchain protecting /bin/ls and writing results into the file
		The Zipr toolchain should report success if installed properly.
		Run the rewritten program as if it were /bin/ls:  "/tmp/ls.ziprd"


If you log out and log back in (or log into a new session), you'll need to re-run steps 1 and 2 
before using the toolchain again.  You may wish to automate this step by adding it to your .bashrc
file or other, appropriate login files.




