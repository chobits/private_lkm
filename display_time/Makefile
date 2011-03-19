obj-m := time.o
CURRENT_PATH :=$(shell pwd)
LINUX_KERNEL_PATH :=/usr/src/kernels/2.6.32.12-115.fc12.i686.PAE
all:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
