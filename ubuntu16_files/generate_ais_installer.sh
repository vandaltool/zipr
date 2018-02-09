#!/bin/bash 

cd $PEASOUP_UMBRELLA_DIR

./regen_install.sh ida ps zipr ubuntu16 stars
cp -r installed zipr_toolchain
tar czf ubuntu16_files/zipr_toolchain.tgz zipr_toolchain
rm -Rf zipr_toolchain
