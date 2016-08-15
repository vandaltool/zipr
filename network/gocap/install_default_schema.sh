#!/bin/bash

data_dir=/var/run/mysql
tmp_dir=/var/run/mysqltmp
snapshot_dir=/usr/local/share/mysql_default_schema
snapshot=mysql.tar.gz

mysql_user=mysql
mysql_group=mysql

rm -rf $data_dir
mkdir -p $data_dir
chown ${mysql_user}:${mysql_group} $data_dir

rm -rf $tmp_dir
mkdir -p $tmp_dir
chown ${mysql_user}:${mysql_group} $tmp_dir

pushd $data_dir
tar -xvz --strip-components 2 -f ${snapshot_dir}/${snapshot}
chown -R ${mysql_user}:${mysql_group} *
popd
