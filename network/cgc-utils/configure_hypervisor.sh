#!/bin/bash

# To use an OpenStack cloud you need to authenticate against the Identity
# service named keystone, which returns a **Token** and **Service Catalog**.
# The catalog contains the endpoints for all services the user/tenant has
# access to - such as Compute, Image Service, Identity, Object Storage, Block
# Storage, and Networking (code-named nova, glance, keystone, swift,
# cinder, and neutron).
#
# *NOTE*: Using the 2.0 *Identity API* does not necessarily mean any other
# OpenStack API is version 2.0. For example, your cloud provider may implement
# Image API v1.1, Block Storage API v2, and Compute API v2.0. OS_AUTH_URL is
# only for the Identity API served through keystone.
export OS_AUTH_URL=http://172.16.8.41:5000/v2.0

# With the addition of Keystone we have standardized on the term **tenant**
# as the entity that owns the resources.
export OS_TENANT_ID=4404c4018791469385b17b6bbf68a251
export OS_TENANT_NAME="admin"
export OS_PROJECT_NAME="admin"

# In addition to the owning entity (tenant), OpenStack stores the entity
# performing the action as the **user**.
export OS_USERNAME="admin"

# With Keystone you pass the keystone password.
export OS_PASSWORD="openstack"

# If your configuration has multiple regions, we set that information here.
# OS_REGION_NAME is optional and only valid in certain environments.
export OS_REGION_NAME="RegionOne"
# Don't leave a blank variable, unset it if it was empty
if [ -z "$OS_REGION_NAME" ]; then unset OS_REGION_NAME; fi

#Checked in like this 
# to ensure that bad defaults
# are not used.
source_vm_name=
src_floating_ip=
dest_ips=

function delete_rules
{
	source_ip="$1"
	shift;
	source_floating_ip="$1"
	shift;
	destination_ips="$1"
	shift;

	echo -n "Deleting existing rules ..."
	old_count=`/sbin/iptables -L -t nat | wc -l`
	new_count=0	
	while [ $old_count -ne $new_count ];
	do
		for destination_ip in $destination_ips;
		do
    			/sbin/iptables -t nat -D nova-network-float-snat -s ${source_ip}/32 -d ${destination_ip} -j SNAT --to-source ${source_floating_ip} > /dev/null 2>&1
		done
		old_count=$new_count
		new_count=`/sbin/iptables -L -t nat | wc -l`
	done
	echo " done."
}

function add_rules
{
	source_ip="$1"
	shift;
	source_floating_ip="$1"
	shift;
	destination_ips="$1"
	shift;

	echo -n "Adding rules ..."
	for destination_ip in $destination_ips;
	do
    		/sbin/iptables -t nat -I nova-network-float-snat -s ${source_ip}/32 -d ${destination_ip} -j SNAT --to-source ${source_floating_ip} > /dev/null 2>&1
		if [ $? -ne 0 ]
		then
			echo "Error while adding rule! Exiting ..."
			exit
		fi
	done
	echo " done"
}

function add_route
{
	echo -n "Adding route ..."
	ip route add 192.168.1.0/24 dev br100
	echo " done"
}

function delete_route
{
	echo -n "Deleting route ..."
	ip route del 192.168.1.0/24 dev br100
	echo " done"
}

function usage
{
	echo "$0 [start-floating|stop-floating|restart-floating]"
	exit
}

if [ $# -ne 1 ];
then
	usage;
fi

case $1 in
	start-floating|stop-floating|restart-floating)
        	src_ip=`nova list | grep ${source_vm_name} | sed 's/^.*cgc_net=\([0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\).*$/\1/'`
	;;
esac

case $1 in
	start-floating)
		sleep 45
		add_rules "$src_ip" "$src_floating_ip" "$dest_ips";
		add_route;
	;;
	stop-floating)
		delete_rules "$src_ip" "$src_floating_ip" "$dest_ips";
		delete_route;
	;;
	restart-floating)
		delete_rules "$src_ip" "$src_floating_ip" "$dest_ips";
		delete_route;
		add_rules "$src_ip" "$src_floating_ip" "$dest_ips";
		add_route;
	;;
	*)
		usage;
	;;
esac
