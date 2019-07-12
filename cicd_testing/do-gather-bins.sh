#/bin/bash

set -x
set -e

mkdir -p test_binaries/

cp $(which ls) ls.$(uname -m)
cp $(which bzip2) ls.$(uname -m)
ls
