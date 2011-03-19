all:module user

module:module/ikey.ko
module/ikey.ko:
	(cd module; make)

user:user/ikey
user/ikey:
	(cd user; make)

install:
	@insmod module/ikey.ko
	@cp user/ikey /usr/bin/ikey

clean:
	(cd module; make clean)
	(cd user; make clean)

uninstall:
	@rmmod ikey
	@rm /usr/bin/ikey

lines:
	./lines.sh
