#!/bin/zsh


hello()
{
	echo hello
	echo what\'s up, duc
	echo $*
}

echo $*
hello bob

while [ true ]; 
do

	hello $*
	break;

done
