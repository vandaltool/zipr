#!/bin/bash 
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#used for filtering program names from output.
ORIG_NAME=tcpdump


#must import the library here, as it depends on some of the above variables
. $TEST_LIB

DELETE_FILTER="stonesoup|gcc|lib|DUMMY|exec|python|tcpdump"

TEST_DIR=$PEASOUP_HOME/tests/tcpdump

run_basic_test 20 -h
run_basic_test 20 -$i -s0 -nr $TEST_DIR/tcpd_tests/print-flags.pcap
run_basic_test 20  -t -n -v -v -v -r $TEST_DIR/tcpd_tests/lmp.pcap 
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/bgp_vpn_attrset.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/mpbgp-linklocal-nexthop.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/eapon1.pcap -t
run_basic_test 20  -t -n -v -v -v -r $TEST_DIR/tcpd_tests/lmp.pcap 
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/02-sunrise-sunset-esp.pcap -t -n
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/02-sunrise-sunset-esp.pcap -t -E "0x12345678@192.1.2.45 3des-cbc-hmac96:0x4043434545464649494a4a4c4c4f4f515152525454575758"
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/08-sunrise-sunset-esp2.pcap -t -E "0x12345678@192.1.2.45 3des-cbc-hmac96:0x43434545464649494a4a4c4c4f4f51515252545457575840,0xabcdabcd@192.0.1.1 3des-cbc-hmac96:0x434545464649494a4a4c4c4f4f5151525254545757584043"
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/02-sunrise-sunset-esp.pcap -t -E "3des-cbc-hmac96:0x4043434545464649494a4a4c4c4f4f515152525454575758"
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/08-sunrise-sunset-esp2.pcap -t -E "file esp-secrets.txt"
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/08-sunrise-sunset-aes.pcap -t -E "file esp-secrets.txt"
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/espudp1.pcap -nnnn -t -E "file esp-secrets.txt"
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/isakmp-delete-segfault.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/isakmp-pointer-loop.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/isakmp-identification-segfault.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/isakmp4500.pcap -t -E "file esp-secrets.txt"
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/mpls-ldp-hello.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/ospf-gmpls.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/ikev2four.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/ikev2four.pcap -t -v -v -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/ikev2four.pcap -t -v -v -v -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/ikev2pI2.pcap -t -E "file ikev2pI2-secrets.txt" -v -v -v -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/dio.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/e1000g.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/forces1.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/forces1.pcap -t -v -v -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/forces1.pcap -t -v -v -v -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/QinQpacket.pcap -t -e
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/QinQpacket.pcap -t -e -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/sflow_multiple_counter_30_pdus.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/babel.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/babel.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/babel_auth.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/pppoe.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/igmpv3-queries.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/icmpv6.pcap -t -vv
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/spb.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/spb_bpduv4.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/ripv1v2.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/ripv2_auth.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/dhcpv6-AFTR-Name-RFC6334.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/dhcpv6-ia-na.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/dhcpv6-ia-pd.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/dhcpv6-ia-ta.pcap -t -v
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/zmtp1.pcap -t -v -T zmtp1
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/msnlb.pcap -t
run_basic_test 20 -n -r $TEST_DIR/tcpd_tests/msnlb2.pcap -t
report_success
