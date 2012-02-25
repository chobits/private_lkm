
all:
	@echo 'Makefile help infomation:'
	@echo ' all      : display current help information'
	@echo ' hello    : generate a simple kernel module object(hello_kernel.ko)'
	@echo ' clean    : clean objects hello_kernel.* in current directory'
	@echo ' clean_all: clean all objects in subdirectores recursively'

# clean all objects
clean_all:
	find . -name "Makefile" -exec make -C `dirname {}` clean ';'

# simple example
obj-m := hello_kernel.o
DEST :=$(shell pwd)
KERN_SRC :=/usr/src/kernels/$(shell uname -r)

hello:
	make -C $(KERN_SRC) M=$(DEST) modules
clean:
	make -C $(KERN_SRC) M=$(DEST) clean
