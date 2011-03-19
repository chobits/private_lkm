#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <linux/fs.h>
#include <fcntl.h>

#include "fool.h"

#define DEFAULT_OFFSET 0

static char *program;

void per(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

void usage(FILE *f)
{
	fprintf(f,
		"Usage:\n"
		" bind device:       %s [-o offset] loopdev file\n"
		" delete device:     %s -d loopdev\n",
		program, program);

	exit(f == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

void delete_loop(const char *device)
{
	int devfd;

	devfd = open(device, O_RDONLY);
	if (devfd == -1) {
		perror("open device");
		return ;
	}

	if (ioctl(devfd, FOOL_CLR_FD, 0) == -1) {
		close(devfd);
		perror("ioctl FOOL_CLR_FD");
		return ;
	}

	close(devfd);
	printf("delete loop device:%s\n", device);
}

void set_loop(char *dev, char *file, unsigned long long offset)
{
	int devfd, fd;
	char *filename;

	devfd = open(dev, O_RDWR);
	if (devfd == -1)
		per("open dev");

	fd = open(file, O_RDWR);
	if (fd == -1)
		per("open file");
	
	/* loop set fd */
	if (ioctl(devfd, FOOL_SET_FD, fd) == -1) {
		close(fd);
		close(devfd);
		per("ioctl FOOL_SET_FD");
	}
	close(fd);
	close(devfd);
}

void get_program_name(char *name)
{
	program = strrchr(name, '/');
	if (program)
		program++;
	else
		program = name;
}

int main(int argc, char **argv)
{
	unsigned long long offset;
	int c, d;
	char *device, *file;

	offset = 0;
	get_program_name(argv[0]);
	
	if (argc == 1)
		usage(stderr);

	d = c = 0;
	while ((c = getopt(argc, argv, "o:d")) != -1) {
		switch (c) {
		case 'd':
			d = 1;
			break;
		case 'h':
			usage(stdout);
			break;
		case '?':
			fprintf(stderr, "unknown option:%s\n", argv[optind-1]);
			exit(EXIT_FAILURE);
			break;
		default:
			usage(stderr);
			break;
		}
	}
	
	device = argv[optind];
	if (optind + 1 >= argc)
		file = NULL;
	else
		file = argv[optind + 1];
	
	/* handle */
	if (d) {
		/* delete */
		while (optind < argc)
			delete_loop(argv[optind++]);
	}
	else if (file) {
		/* bind device to file */
		set_loop(device, file, offset);
	}
	else {
		/* show loop device infomation */
		usage(stderr);
	}

	return 0;
}
