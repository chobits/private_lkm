obj-m := chr_n.o
CURRENT_PATH := $(shell pwd)
LINUX_KERNEL_PATH := /usr/src/kernels/$(shell uname -r)
# make -C /xx  <=>cd /xx; make
all:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
i:
	sudo insmod chr_n.ko
	sudo mknod cat c 248 0
u:
	sudo rmmod chr_n
	sudo rm -f cat
