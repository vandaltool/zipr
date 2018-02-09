#/bin/bash

./generate_ais_installer.sh
vagrant destroy -f 
vagrant up
