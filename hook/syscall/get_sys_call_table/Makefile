obj-m := get.o
CURRENT_PATH :=$(shell pwd)
LINUX_KERNEL_PATH :=/usr/src/kernels/$(shell uname -r)

#the module name will be chrdev.o. Kbuild will
#compile the objects listed in $(chrdev-objs) and then run
#"$(LD) -r" on the list of these files to generate chrdev.o.
#hello-objs := chrdev.o chrdev2.o

# make -C /xx  <=>cd /xx; make
all:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
