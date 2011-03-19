obj-m := asm_hlt.o
CURRENT_PATH :=$(shell pwd)
LINUX_KERNEL_PATH :=/usr/src/kernels/2.6.31.5-127.fc12.i686.PAE
# make -C /xx  <=>cd /xx; make
all:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
i:
	sudo insmod chr_dev.ko
	sudo mknod /dev/test_my c 248 0
u:
	sudo rm /dev/test_my
	sudo rmmod chr_dev
