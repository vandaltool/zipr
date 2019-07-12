#/bin/bash

set -x
set -e

mkdir -p artifacts/test_binaries/

cp $(which ls) artifacts/test_binaries/ls.$(uname -m)
cp $(which bzip2) artifacts/test_binaries/bzip2.$(uname -m)
ls
