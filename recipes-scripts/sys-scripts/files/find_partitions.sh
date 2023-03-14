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

if [[ -c "/dev/ubi1_0" ]]; then
    echo "Mounting the baseband firmware partition specified by the bootloader"
    mount -t ubifs -o ro /dev/ubi1_0 /firmware
else
    echo "Bootloader didn't pass the modem partition number, trying to find it"
    modem_partition_id=`cat /proc/mtd | grep -i modem | sed 's/^mtd//' | awk -F ':' '{print $1}'`
    if [[ $modem_partition_id -ge 1 ]]; then 
        echo "Attaching and mounting"
        ubiattach -m $modem_partition_id -d 1 /dev/ubi_ctrl 
        mount -t ubifs -o ro /dev/ubi1_0 /firmware
    else
        echo "Can't mount the baseband firmware partition!"
    fi
fi

# Tell the Hexagon it can boot once the firmware is in place
if [ -f "/sys/kernel/boot_adsp/boot" ]; then
    echo "Booting ADSP on the downstream kernel"
    echo 1 > /sys/kernel/boot_adsp/boot
fi

# For Mainline: Tell remoteproc0 to boot once the partition has been mounted
if [ -f "/sys/class/remoteproc/remoteproc0/state" ]; then
    echo "We're booting a mainline kernel!"
    echo start > /sys/class/remoteproc/remoteproc0/state
fi

# We have 10 seconds of spare time between adsp boot signal to actually
# ready, so after telling it to boot we mount persistent storage
user_data_partition_id=`cat /proc/mtd | grep -i usr_data | sed 's/^mtd//' | awk -F ':' '{print $1}'`
if [[ $user_data_partition_id -ge 1 ]]; then 
    echo "Mounting user data partition"
    ubiattach -m $user_data_partition_id -d 2 /dev/ubi_ctrl 
    mount -t ubifs -o rw /dev/ubi2_0 /persist
else
    echo "Mounting system root partition as RW"
    mount -o remount,rw /
fi
