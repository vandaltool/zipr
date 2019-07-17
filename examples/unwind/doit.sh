#/bin/bash

function main()
{
	set -e
	set -x
	g++ unc.c -o unc.exe
	$PSZ unc.exe unc-zipr.exe --tempdir unc-temp

	./unc.exe
	./unc-zipr.exe

	rm -rf unc.exe unc-zipr.exe unc-temp


}

main "$@"
