#!/bin/bash
# Caculate code lines:)

if [ $# = 1 ] && [ $1 = -h ]
then
	echo "Usage:$0 [OPTION] [FILE/DIR]"
	echo "Option:"
	echo "     h     help information"
	exit 0
fi

wc -l `find $@| grep "\.[csSh]$"`
