all:ikey
ikey:keymain.o nlswitch.o readlog.o
	gcc $^ -o $@

keymain.o:keymain.c
	gcc $< -c -o $@

nlswitch.o:nlswitch.c
	gcc $< -c -o $@

readlog.o:readlog.c
	gcc $< -c -o $@

clean:
	rm *.o ikey
