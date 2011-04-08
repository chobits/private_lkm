obj-m := hello_workqueue.o
CURRENT_PATH :=$(shell pwd)
LINUX_KERNEL_PATH :=/usr/src/kernels/2.6.31.5-127.fc12.i686.PAE
all:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
