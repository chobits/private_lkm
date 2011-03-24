obj-m := mem.o
CP :=$(shell pwd)
#LP :=/usr/src/linux
LP :=/usr/src/kernels/`uname -r`

all:
	make -C $(LP) M=$(CP) modules
clean:
	make -C $(LP) M=$(CP) clean
