#!/bin/bash

apt-get update
apt-get install git
apt-get install libpcap-dev

pushd /home/team/
mkdir go
mkdir gocode

pushd go
wget https://storage.googleapis.com/golang/go1.6.2.linux-amd64.tar.gz
tar -xvzf go1.6.2.linux-amd64.tar.gz
cat >> /home/team/.bashrc <<HERE
export GOPATH=/home/team/gocode/
export GOROOT=/home/team/go/go/
export PATH=\$GOROOT/bin/:\$PATH
HERE
export GOPATH=/home/team/gocode/
export GOROOT=/home/team/go/go/
export PATH=$GOROOT/bin/:$PATH

go get github.com/google/gopacket
go get github.com/go-sql-driver/mysql
go get github.com/kr/beanstalk
go get gopkg.in/yaml.v2
popd
popd

cat >> /root/.bashrc <<HERE
export GOPATH=/home/team/gocode/
export GOROOT=/home/team/go/go/
export PATH=\$GOROOT/bin/:\$PATH
HERE
