obj-m := hello.o
CURRENT_PATH :=$(shell pwd)
LINUX_KERNEL_PATH :=/usr/src/kernels/$(shell uname -r)

#the module name will be chrdev.o. Kbuild will
#compile the objects listed in $(chrdev-objs) and then run
#"$(LD) -r" on the list of these files to generate chrdev.o.
#hello-objs := chrdev.o chrdev2.o

# make -C /xx  <=>cd /xx; make
# we can also use SUBDIRS instead of M
# but M take precedence!
all:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean

#generate all-no config
# make allnoconfig
#how to make module in kernel-src:
# make M=fs/ext4 modules ==> ext4.ko
# make M=fs/ext4 clean
