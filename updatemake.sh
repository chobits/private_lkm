#!/bin/bash
if [ $# != 2 ]
then
	echo "Usage: $0 makefile dir"
	exit 0
fi
makefile=$1
dir=$2
makes=`find $dir |grep -w Makefile`
for f in $makes
do
	cp $makefile $f
	make -C `dirname $f` clean
done
