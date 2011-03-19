#!/bin/bash
echo "ls dev"
ls -l /dev |grep led
echo "/proc/devices"
cat /proc/devices |grep led
echo "lsmod"
lsmod |grep led
