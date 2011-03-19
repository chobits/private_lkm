obj-m := htool.o
tool-m := htool.ko
S_TABLE = 0x`grep sys_call_table /proc/kallsyms| head -c 8`
KERNELBUILD :=/lib/modules/`uname -r`/build

default:
	@echo
	@echo "\t-----------------------------------------------------"
	@echo "\t\tH-tool by hedgehog"
	@echo "\t\tblog.csdn.net/littlehedgehog"
	@echo "\t-----------------------------------------------------"
	@echo "\t\tTHANK TO 常涛 @ comm1.hdu.edu.cn/xa"
	@echo "\t-----------------------------------------------------"
	@echo
	@echo "#define HIJACK_SYS_CALL_TABLE $(S_TABLE) " > syscall.h
	make -C $(KERNELBUILD) M=$(shell pwd) modules
	gcc -o w winona.c -Wall


install:
	@echo
	@echo "\t-----------------------------------------------------"
	@echo "\t\tH-tool by hedgehog"
	@echo "\t\tblog.csdn.net/littlehedgehog"
	@echo "\t-----------------------------------------------------"
	@echo "\t\tTHANK TO 常涛 @ comm1.hdu.edu.cn/xa"
	@echo "\t-----------------------------------------------------"
	@echo
	
	@sudo sh shell
	@sudo mkdir /etc/.h-tool
	@sudo cp $(tool-m) /etc/.h-tool/
	@sudo touch /etc/.h-tool/.h_filter
	@sudo chmod 0777 /etc/.h-tool/.h_filter
	@sudo cp /etc/rc.local /etc/rc.local.backup
	@sudo echo "#!/bin/sh -e" > /etc/rc.local
	@sudo echo "insmod /etc/.h-tool/$(tool-m)" >> /etc/rc.local
	@sudo echo "exit(0)" >> /etc/rc.local
	@sudo insmod $(tool-m)
	@echo
	@echo "H-tool installed successfully! Enjoy watching your adult video!"
	@echo

uninstall:
	@echo
	@echo "\t-----------------------------------------------------"
	@echo "\t\tH-tool by hedgehog"
	@echo "\t\tblog.csdn.net/littlehedgehog"
	@echo "\t-----------------------------------------------------"
	@echo "\t\tTHANK TO 常涛 @ comm1.hdu.edu.cn/xa"
	@echo "\t-----------------------------------------------------"
	@echo

	@sudo rm -rf /etc/.h-tool
	@sudo rm /etc/rc.local
	@sudo cp /etc/rc.local.backup /etc/rc.local
	@sudo insmod $(tool-m)

	@echo 
	@echo "H-tool uninstalled successfully! Restart your computer"
	@echo ""


clean:
	rm -rf *.o *.ko *.mod.c .*.cmd *.orig .tmp_versions
in:
	sudo insmod $(tool-m)
out:
	sudo rmmod $(tool-m)

