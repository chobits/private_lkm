#!/bin/bash
MAJOR=`grep xmem /proc/devices | cut -f1 -d" "`

if [ "$1" = "-c" ]
then
	if [ -c xzero ] || [ -c xnull ]
	then
		echo "err: already created"
		exit 0
	fi
	sudo mknod -m 0666 xzero c $MAJOR 0 
	sudo mknod -m 0666 xnull c $MAJOR 1
	echo "create node: xzero xnull"
elif [ "$1" = "-r" ]
then
	if [ ! -c xzero ] && [ ! -c xnull ]
	then
		echo "err: not created"
		exit 0
	fi
	sudo rm xzero xnull
	echo "remove node: xzero xnull"
else
	echo "Usage : `basename $0` option"
	echo "option:"
	echo "        -c       create nodes"
	echo "        -r       remove nodes"
fi
