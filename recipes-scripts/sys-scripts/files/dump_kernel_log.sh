#!/bin/sh
mount -o remount,rw /persist
echo "Dumping kernel log..." >> /persist/kernel_log
dmesg >> /persist/kernel_log
echo "Dump complete." >> /persist/kernel_log
sync