#/bin/sh 


if [ -z "$1"  ]; then
	echo "usage: $0 <externals config file>"
fi

svn propset svn:externals . -F $1
svn up
