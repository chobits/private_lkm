obj-m := ikey.o
ikey-objs := keymain.o keylog.o keyswitch.o
CP := $(shell pwd)
LP :=/usr/src/kernels/`uname -r`

all:
	make -C $(LP) M=$(CP) modules
clean:
	make -C $(LP) M=$(CP) clean

