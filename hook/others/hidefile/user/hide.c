#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "hentry.h"

void dprint(const struct hentry *he, char *filename)
{
	if (!he)
		return ;
	
	printf("[h-usr] ");
	switch (he->flag) {
	case HIDE_ADD:
		printf("hide ");
		break;
	case HIDE_DEL:
		printf("show ");
		break;
	default:
		/* should be here? */
		break;
	}
	printf("filename: %s ino: %llu\n", filename, he->ino);

}

void per(char *str)
{
	if (str)
		perror(str);
	else
		perror("ERR");
	exit(1);
}

void usage(void)
{
	fprintf(stderr, "hide [h/d] filename\n");
	exit(1);
}

int main(int argc, char **argv)
{
	struct stat sbuf;
	struct hentry he;
	int fd, n;
	
	if (argc != 3)
		usage();
	/* stat can find file, although the file is hiden */
	if (stat(argv[2], &sbuf) == -1)
		per("fstat");
	if ((fd = open(HIDE_PROC_ENTRY, O_WRONLY)) == -1)
		per("open");
		
	if (strncmp(argv[1], "h", 1) == 0)
		he.flag = HIDE_ADD;
	else if (strncmp(argv[1], "d", 1) == 0)
		he.flag = HIDE_DEL;
	else
		usage();

	he.magic = HIDE_MAGIC;
	he.ino = sbuf.st_ino;

	dprint(&he, argv[2]);	

	if ((n = write(fd, &he, sizeof(he))) != sizeof(he))
		per("write");
	return 0;
}
