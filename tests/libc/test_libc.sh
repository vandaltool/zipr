#!/bin/bash


main()
{
	set -e
	set -x

	#  make sure no prior libcs are hanging out.
	rm -rf libc.* || true

	# calc some names
	local libc=$(ldd $(which ls)|grep 'libc\.'|cut -d'>' -f2|cut -d'(' -f1)
	local libc_short=$(basename $libc)
	local libc_zipr=$(basename $libc).zipr

	# run zipr
	$PSZ $(realpath $libc) $libc_zipr

	# move the zir'd libc to the right name.  All subsequent commands
	# now run with the zipr'd libc
	mv $libc_zipr $libc_short

	# invoke gcc
	gcc hello.c || exit 1
	./a.out || exit 1

	# invoke zipr on ls -- using zipr'd libc.
	$PSZ $(which ls) ls.zipr || exit 1
	./ls.zipr -lhrSR || exit 1
	ldd ./ls.zipr
}

main "$@"

