#/bin/bash

print_usage()
{
	echo ""
	echo " This docker container is made available to the public by Zephyr Software       "
	echo " (contact: jwd@zephyr-software.com) under the Creative Commons Attribution-     "
	echo " NonCommercial license (CC BY-NC).                                              "
	echo ""
	echo " Linux, Gcc, and other relevant open source projects are licensed under their   "
	echo " own license and are exempt from this license statement.                        "
	echo ""
	echo "IRDB toolchain subcommands:"
	echo ""
	echo "	iagree	        Accept the creative commons non-commercial license and login."
	echo "	help            Print this menu."
	echo ""
}


main()
{
	local res=0
	local subcommand=$1

	shift
	
	case "$subcommand" in
		iagree)
			echo
			echo Welcome to the IRDB toolchain docker image!
			echo
			echo 'The IRDB toolchain is setup and ready to run.'
			echo 'You could start your first experiment with:'
			echo
			echo 'zuser@a3fc1666aaa4:~$ pszr /bin/ls ./ls.bilr'
			echo 'Using Zipr backend.'
			echo 'Detected ELF shared object.'
			echo 'Performing step rida [dependencies=mandatory] ...Done.  Successful.'
			echo 'Performing step pdb_register [dependencies=mandatory] ...Done.  Successful.'
			echo 'Performing step fill_in_cfg [dependencies=unknown] ...Done.  Successful.'
			echo 'Performing step fill_in_indtargs [dependencies=unknown] ...Done.  Successful.'
			echo 'Performing step fix_calls [dependencies=unknown] ...Done.  Successful.'
			echo 'Performing step zipr [dependencies=none] ...Done.  Successful.'
			echo 'zuser@a3fc1666aaa4:~$ ./ls.bilr -l ' 
			echo ' < ls output >  '
			echo 'zuser@a3fc1666aaa4:~$ readelf -l /bin/ls ./ls.p1 ' 
			echo
			bash
			res=0
		;;

		help)
			print_usage
			exit 0
		;;
		*)
			print_usage
			echo 
			echo "Unknown subcommand: '$subcommand'"
			echo 
			exit 1
		;;
	esac

	if [[ $res  != 0 ]]; then
		echo
		echo Subcommand failed.  Logs were printed.
		exit 1
	fi
	exit 0
}

main "$@" 
