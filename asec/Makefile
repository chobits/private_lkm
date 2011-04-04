obj-m := asec.o
asec-objs := asec_lsm.o asecfs_if.o file.o

CP :=$(shell pwd)
LP :=/usr/src/linux

all:
	make -C $(LP) M=$(CP) modules
clean:
	make -C $(LP) M=$(CP) clean
