#/bin/bash

function main()
{
	set -e
	set -x
	sudo apt-get install libunwind-dev -y
	g++ unc.c -o unc.exe -lunwind
	$PSZ unc.exe unc-zipr.exe --tempdir unc-temp

	./unc.exe
	./unc-zipr.exe

	rm -rf unc.exe unc-zipr.exe unc-temp
}

main "$@"
