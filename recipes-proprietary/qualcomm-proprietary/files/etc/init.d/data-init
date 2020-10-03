#! /bin/sh

#Copyright (c) 2012,2014 Qualcomm Technologies, Inc.  All Rights Reserved.
#Qualcomm Technologies Proprietary and Confidential.

#The init script sets the following procfs entry to 1
#/proc/sys/net/netfilter/nf_conntrack_tcp_be_liberal
#to prevent out of order acknowledgements from making
#connection tracking to treat them as not belonging to
#the connection they belong to. Otherwise, a weird issue
#happens in which some long connections on high-throughput
#links get dropped when an ack packet comes out of order.

echo 1 > /proc/sys/net/netfilter/nf_conntrack_tcp_be_liberal

#The init script  disables the nf_conntrack_checksum
#this would disable checksum computation for all received
#pkts before forwarding.Checksum computaiton would be done
#by TCPIP stack for embedded case and by the client for
#tethered/WLAN clients. the motivaiton for this change is
#to improve MIPS and E2E tput for SW data path.

echo 0 > /proc/sys/net/netfilter/nf_conntrack_checksum
