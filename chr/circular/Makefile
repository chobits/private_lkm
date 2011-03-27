obj-m := circular.o
CP :=$(shell pwd)
LP :=/usr/src/kernels/$(shell uname -r)

all:
	make -C $(LP) M=$(CP) modules
clean:
	make -C $(LP) M=$(CP) clean

