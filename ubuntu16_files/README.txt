
Steps to installing.

	0) Unpack the tarball (likely called zipr_toolchain.tgz) -- you must have done this already 
		to be reading this!
	1) Switch to the directory created during unpacking. i.e., cd zipr_toolchain
	2) Copy your ida.key for IdaPro7.0 to ./idaproCur/ida.key
	3) Run "./ubuntu16_files/install.sh" in this directory -- lots of packages 
		will be installed.
	4) Run "source set_env_vars"
	5) This step sets up postgres to hold the Zipr database.  If you already 
		have postgres configured, it may damage your data.  Use with appropriate caution.
		Run "postgres_setup.sh"
	6) Test  the installation:
		run "cd /tmp; $PSZ /bin/ls ./ls.ziprd"
		You should see the Zipr toolchain protecting /bin/ls and writing results into the file
		The Zipr toolchain should report success if installed properly.
		Run the rewritten program as if it were /bin/ls:  "/tmp/ls.ziprd"


If you log out and log back in (or log into a new session), you'll need to re-run steps 1 and 4 
before using the toolchain again.  You may wish to automate this step by adding it to your .bashrc
file or other, appropriate login files.




