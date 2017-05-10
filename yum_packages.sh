#/bin/bash 

packs="
scons
cmake
nasm
elfutils-libelf-devel
postgresql
glibc-devel.i686 
glibc-devel
jsoncpp
jsoncpp-devel
"

groups="Development Tools"

sudo yum -y --enablerepo=extras install epel-release

for i in $groups
do
	sudo yum -y groupinstall $groups
done


for i in $packs
do
	sudo yum -y install $packs
done

