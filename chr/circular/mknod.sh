#!/bin/bash
MAJOR=`grep circular /proc/devices | cut -f1 -d" "`

if [ "$1" = "-c" ]
then
	if [ -c cir ] 
	then
		echo "err: already created"
		exit 0
	fi
	mknod cir c $MAJOR 0 
	echo "create node: cir"
elif [ "$1" = "-r" ]
then
	if [ ! -c cir ]
	then
		echo "err: not created"
		exit 0
	fi
	rm cir
	echo "remove node: cir"
else
	echo "Usage : `basename $0` option"
	echo "option:"
	echo "        -c       create node"
	echo "        -r       remove node"
fi
