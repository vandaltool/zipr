#/bin/bash

set -x
set -e

mkdir -p test_binaries/

cp $(which ls) test_binaries/ls.$(uname -m)
cp $(which bzip2) test_binaries/ls.$(uname -m)
ls
