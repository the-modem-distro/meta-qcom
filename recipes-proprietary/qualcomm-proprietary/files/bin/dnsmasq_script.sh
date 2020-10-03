#!/bin/sh
#Copyright (c) 2015-2016 Qualcomm Technologies, Inc.  All Rights Reserved.
#Qualcomm Technologies Proprietary and Confidential.
#
#script run by dnsmasq on providing IP
#help
# dnsmasq option --dhcp-script=<path of script> with provide
# command line argument in below format
#
#  event mac                ip             hostname
#  add   b2:5b:c5:66:77:8b  192.168.225.45 lab3284
#  $1    $2                 $3             $4
#
#creating file to store dnsmasq info
FILE="/tmp/dnsmasq_host.txt"

if [ ! -f  $FILE ] ; then
  #File  does not exists creating
  touch $FILE
  chmod a+rw $FILE
fi

case "$1" in
  add)
      #"add" event  means a lease has been created
      #Removing entry for that is already for  ip=$3
      sed -i "/$3/d" $FILE
      echo "$2 $3 $4" >> $FILE   # storing entry with mac ip & hostname
      ;;
  old)
      #"old" is a notification of an existing lease when dnsmasq starts
      # or a change to MAC address or hostname of an existing lease
      #checking for if we have entry stored in file or not

      if grep -q "$2 $3" "$FILE"     # checking entry match  with ip & mac only
      then
        # match if found
        # for match entry dnsmasq sometimes will not provide hostname
        echo "match found" > /dev/null 2>&1
      else
        # match if not found & hostname not empty
        if [ "$4" != "" ]
        then
        #first Removing entry for that is already for  ip=$3
        sed -i "/$3/d" $FILE
        #match not found so adding to file
        echo "$2 $3 $4" >> $FILE # storing entry with mac ip & hostname
      fi

      fi
      ;;
  del)
      #del event means entry has been destroyed by dnsmasq
      #Removing entry for ip=$3
      sed -i "/$3/d" $FILE
      ;;
  *)
    #invalid event

    ;;
esac

