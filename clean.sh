#!/bin/bash
makes=`find .|grep -w Makefile`
for f in $makes
do
	make -C `dirname $f` clean
done
