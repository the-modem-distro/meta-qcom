#!/bin/sh
# Copyright (c) 2014, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# find_partitions        init.d script to dynamically find partitions
#
if [ ! -d /data ];then
	 mount -o remount,rw /
	 mkdir -p /data
 	 mount -o remount,ro /

fi
if [ ! -d /persist ];then
	 mount -o remount,rw /
	 mkdir -p /persist
 	 mount -o remount,ro /
fi
if [ ! -d /firmware ];then
	 mount -o remount,rw /
	 mkdir -p /firmware
 	 mount -o remount,ro /
fi
# Attach the modem (11) and data (14) ubifs
ubiattach -m 11 -d 1 /dev/ubi_ctrl
# ubiattach -m 14 -d 2 /dev/ubi_ctrl
sleep 1
mount -t ubifs -o ro /dev/ubi1_0 /firmware
# mount -t ubifs /dev/ubi2_0 /persist
mount -t tmpfs -o size=8m tmpfs /data
# Tell the Hexagon it can boot once the firmware is in place
echo 1 > /sys/kernel/boot_adsp/boot
