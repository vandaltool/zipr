#!/bin/bash

remote_directory=/tmp/`hostname`_$$

check_env()
{
	if [ -z "$PSZHOST" ]; then 
		echo Failed to set PSZHOST
		exit 2
	fi
	if [ -z "$PSZUSER" ]; then
	    PSZUSER=`whoami`
	fi
	if [ -z "$PSZPORT" ]; then
	    PSZPORT=22
	fi

}



do_setup_remote_dir()
{
	local infile=$1

	# Create unique directory on server
	ssh -p $PSZPORT $PSZUSER@$PSZHOST mkdir -p $remote_directory > /dev/null 2>&1 

	# copy infile to it.
	scp -P $PSZPORT $infile $PSZUSER@$PSZHOST:$remote_directory/in.exe > /dev/null 2>&1 

}


do_protect()
{
	local args="$@"

	# Run $PSZ
	ssh -p $PSZPORT $PSZUSER@$PSZHOST "cd ~/umbrellas/peasoup_umbrella/; . set_env_vars ; cd $remote_directory; \$PSZ in.exe out.exe $args --tempdir ps_tmp"
	local res=$?
	echo "Protection result: $res"
	if [[ $res != 0 ]]; then
		echo "Failed.  Remote dir is: $remote_directory"
		exit 1
	fi

}

do_get_results()
{
	local outfile=$1
	scp -P $PSZPORT $PSZUSER@$PSZHOST:$remote_directory/out.exe $outfile > /dev/null 2>&1 
	scp -r -P $PSZPORT $PSZUSER@$PSZHOST:$remote_directory/ps_tmp ps_temp.$$ > /dev/null 2>&1 
}

do_cleanup()
{
	# Cleanup
	ssh -p $PSZPORT $PSZUSER@$PSZHOST rm -rf $remote_directory
}

main()
{
	local infile=$1
	local outfile=$2
	shift 2
	opts="$@"
	check_env
	do_setup_remote_dir $infile
	do_protect "$opts"
	do_get_results $outfile
	do_cleanup
}

main "$@"

