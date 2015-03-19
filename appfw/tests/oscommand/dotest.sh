#!/bin/bash -x

APPFW_LIB=$SECURITY_TRANSFORMS_HOME/appfw/lib/libappfw.so64

for i in {1..11}
do
	
	clear
	echo ----------------- unmodified: input \"$i Makefile\" and  \"$i -l\"
	./testosc.exe $i Makefile
	./testosc.exe $i -l
	echo ----------------- peasoup
	LD_PRELOAD=$APPFW_LIB APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe $i Makefile
	LD_PRELOAD=$APPFW_LIB APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe $i -l
	echo Press [enter] to continue ...
	read name

	
done


for i in {101..111}
do
	
	clear
	echo ----------------- unmodified: input \"$i Makefile\" and  \"$i -l\"
	./testosc.exe $i env | head -5
	./testosc.exe $i -i env 
	echo ----------------- peasoup
	LD_PRELOAD=$APPFW_LIB APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe $i env |head -5
	LD_PRELOAD=$APPFW_LIB APPFW_DB=appfw.db APPFW_SIGNATURE_FILE=testosc.exe.sigs ./testosc.exe $i -i env
	echo Press [enter] to continue ...
	read name

	
done
