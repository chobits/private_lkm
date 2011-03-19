#!/bin/bash
usage() {
	echo "usage: `basename $0` [on|off]"
}

if [ $# != "1" ]
then
	usage
	exit 0
fi

if [ $1 == "on" ]
then
	mkfs.ext2 /dev/memd0
	mount /dev/memd0 blkdir
elif [ $1 == "off" ]
then
	umount blkdir
	rmmod blk
else
	usage
fi
