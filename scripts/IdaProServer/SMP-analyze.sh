#!/bin/bash

if [ -z "$IDA_PRO_SERVER_HOST" ]; then echo Failed to set IDA_PRO_SERVER_HOST; exit 2; fi
if [ -z "$IDA_PRO_SERVER_USER" ]; then
    IDA_PRO_SERVER_USER=`whoami`
fi
if [ -z "$IDA_PRO_SERVER_PORT" ]; then
    IDA_PRO_SERVER_PORT=22
fi

directory=/tmp/`hostname`

# Create unique directory on server
ssh -p $IDA_PRO_SERVER_PORT $IDA_PRO_SERVER_USER@$IDA_PRO_SERVER_HOST mkdir $directory

echo "--DEBUG---------"
pwd
echo "---"
$SMPSA_HOME/findso.pl $@
echo "---"
echo $LD_LIBRARY_PATH
echo "---"
ldd $@
echo "--END DEBUG---------"


# Copy my files to it
# The perl script will also include names of dependent shared object (.so) file
# Start timing stuff
copyStartTime=`date +%s`
scp -P $IDA_PRO_SERVER_PORT -q $@ `$SMPSA_HOME/findso.pl $@` $IDA_PRO_SERVER_USER@$IDA_PRO_SERVER_HOST:$directory
if [ -n "$SMPSA_PLUGIN" ]; then
    scp -P $IDA_PRO_SERVER_PORT -q ${SMPSA_PLUGIN}* $IDA_PRO_SERVER_USER@$IDA_PRO_SERVER_HOST:$directory
fi

copyStopTime=`date +%s`
# Check to see if the Ida Pro Server is too busy now and wait if necc.
if [ -n "$MAX_IDA_PROCESSES" ]; then
    while [ `ssh -p $IDA_PRO_SERVER_PORT $IDA_PRO_SERVER_USER@$IDA_PRO_SERVER_HOST pgrep idal|wc -l` -ge "$MAX_IDA_PROCESSES" ]; do
        random=`od -An -N2 -tu2 /dev/urandom`
        # Wait 10-30 seconds
        seconds=`expr $random % 20 + 10`
        echo Waiting $seconds seconds for an IDA process to exit...
        sleep $seconds
    done
fi

# waitStartTime = copyStopTime
waitStopTime=`date +%s`

# Run ida pro on server
# \time tells the shell to use /usr/bin/time instead of the buil-in time command
# /usr/bin/time reports the time on stderr. The commands below set executeTime to the time reported by /usr/bin/time
if [ -z "$SMPSA_PLUGIN" ]; then
	ssh -p $IDA_PRO_SERVER_PORT $IDA_PRO_SERVER_USER@$IDA_PRO_SERVER_HOST "cd peasoup; source set_env_vars; export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.; cd $directory; screen -D -L -ln -m -a -T xterm sh -x "'$SMPSA_HOME'"/SMP-analyze.sh $@" 2>&1
else
	ssh -p $IDA_PRO_SERVER_PORT $IDA_PRO_SERVER_USER@$IDA_PRO_SERVER_HOST "cd peasoup; source set_env_vars; export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.; export SMPSA_PLUGIN=$directory/SMPStaticAnalyzer.plx; cd $directory; screen -D -L -ln -m -a -T xterm sh -x "'$SMPSA_HOME'"/SMP-analyze.sh $@; rm -f $directory/SMPStaticAnalyzer.plx" 2>&1
fi

copyAnswerStartTime=`date +%s`
# Copy the answer back
scp -P $IDA_PRO_SERVER_PORT $IDA_PRO_SERVER_USER@$IDA_PRO_SERVER_HOST:$directory/$@.* .

copyAnswerStopTime=`date +%s`

# Calculate times
#copyTime=$(expr $copyStopTime - $copyStartTime)
#waitTime=$(expr $waitStopTime - $copyStopTime)
#copyAnswerTime=$(expr $copyAnswerStopTime - $copyAnswerStartTime)

# write to file
#echo "Copy Time, Wait Time, Execute Time, Copy Answer Time" >> /tmp/x.x
#echo "`hostname`,$copyTime,$waitTime,$executeTime,$copyAnswerTime" >> /tmp/`hostname`-results.txt

# Cleanup
#ssh -p $IDA_PRO_SERVER_PORT $IDA_PRO_SERVER_USER@$IDA_PRO_SERVER_HOST rm -rf $directory
