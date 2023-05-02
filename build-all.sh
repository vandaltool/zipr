#!/bin/bash



if [[ "$*" =~ "--debug" ]]; then
	SCONSDEBUG=" debug=1 "
fi


if [[ $(uname -m) == 'armv7l' ]] || [[ $(uname -m) == 'aarch64' ]]; then
	scons $SCONSDEBUG debug=1 
else 
	scons $SCONSDEBUG -j 3
fi

exit
