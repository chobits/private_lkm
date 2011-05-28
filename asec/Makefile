obj-m := asec.o
asec-objs :=	asec_lsm.o\
		asecfs_if.o\
		asecfs_policy.o\
		asecfs_query.o\
		file.o\
		asec_iostream.o

CP :=$(shell pwd)
LP :=/usr/src/kernels/$(shell uname -r)

all:
	make -C $(LP) M=$(CP) modules
clean:
	make -C $(LP) M=$(CP) clean
