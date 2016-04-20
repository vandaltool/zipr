#!/bin/bash  -x

#
# copy from local install to remote target binaries, files, scripts that may need an update
#
# assumption: directory layout on target already setup properly
#
#

LOCAL_UMBRELLA_DIR=$PEASOUP_UMBRELLA_DIR
REMOTE_UMBRELLA_DIR=$1

if [ -z ${REMOTE_UMBRELLA_DIR} ]; then
	echo "Must specify the remote peasoup umbrella dir"
	echo "as a local path, or a remote path <host>:</path>"
	exit 1
fi


# Copy umbrella own scripts
scp -r ${LOCAL_UMBRELLA_DIR}/*.sh ${REMOTE_UMBRELLA_DIR}/
scp -r ${LOCAL_UMBRELLA_DIR}/set* ${REMOTE_UMBRELLA_DIR}/
scp -r ${LOCAL_UMBRELLA_DIR}/*vars* ${REMOTE_UMBRELLA_DIR}/

# Zipr
scp -r ${LOCAL_UMBRELLA_DIR}/zipr_install ${REMOTE_UMBRELLA_DIR}/

# Security transforms
scp -r ${LOCAL_UMBRELLA_DIR}/security_transforms/bin ${REMOTE_UMBRELLA_DIR}/security_transforms/
scp -r ${LOCAL_UMBRELLA_DIR}/security_transforms/lib ${REMOTE_UMBRELLA_DIR}/security_transforms/
scp -r ${LOCAL_UMBRELLA_DIR}/security_transforms/plugins_install ${REMOTE_UMBRELLA_DIR}/security_transforms/

# Peasoup scripts
scp ${LOCAL_UMBRELLA_DIR}/peasoup_examples/tools/*.sh ${REMOTE_UMBRELLA_DIR}/peasoup_examples/tools/
scp ${LOCAL_UMBRELLA_DIR}/peasoup_examples/tools/db/*.sh ${REMOTE_UMBRELLA_DIR}/peasoup_examples/tools/db/
scp ${LOCAL_UMBRELLA_DIR}/peasoup_examples/tools/db/*.tbl ${REMOTE_UMBRELLA_DIR}/peasoup_examples/tools/db/

# IDA Plugin
scp ${LOCAL_UMBRELLA_DIR}/idaproCur/plugins/SMPStaticAnalyzer.plx64 ${REMOTE_UMBRELLA_DIR}/idaproCur/plugins/

# Daffy
scp ${LOCAL_UMBRELLA_DIR}/daffy/*.sh ${REMOTE_UMBRELLA_DIR}/daffy/
scp ${LOCAL_UMBRELLA_DIR}/daffy/layout-analysis/analyzer ${REMOTE_UMBRELLA_DIR}/daffy/layout-analysis/

