#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define QUERY_FILE "/sys/kernel/security/asec/query"

void perx(char *str)
{
	if (str) {
		if (errno)
			perror(str);
		else
			fprintf(stderr, "%s\n", str);
	}

	exit(EXIT_FAILURE);
}

int xopen(const char *file, int flags)
{
	int fd;
	fd = open(file, flags);
	if (fd == -1)
		perx("open");
	return fd;
}

unsigned int get_answer(void)
{
	unsigned int answer;
	char *str_yes = "yY";
	char buf[128] = { };
	fflush(stderr);
	fflush(stdout);
	if (!fgets(buf, 256, stdin))
		perx("fgets");
	if (strchr(str_yes, buf[0])) {
		/* allow */
		answer = 1;	
	} else {
		/* not allow */
		answer = 2;	
	}
	return answer;
}

void pollhandle(int fd)
{
	char qbuf[128];
	unsigned int syncnum, answer;
	int n;
	n = read(fd, qbuf, 128);
	if (n < 0)
		perx("read from query");
	if (sscanf(qbuf, "Q syncnum=%u", &syncnum) != 1) {
		fprintf(stderr, "get error query:%s\n", qbuf);	
		exit(EXIT_FAILURE);	
	}
	printf("Do you allow this operation?('Y'es/Others)\n");		
	answer = get_answer();
	n = snprintf(qbuf, 128, "A syncnum=%u answer=%u\n", syncnum, answer);
	if (write(fd, qbuf, n) != n)
		perx("write query");
}

int main(void)
{
	struct pollfd pfd = { };
	int fd, n;

	/* open file */
	fd = open(QUERY_FILE, O_RDWR);
	if (fd == -1) {
		if (errno == EACCES)
			fprintf(stderr, "ERROR:Only one user can watch %s!\n",
								QUERY_FILE);
		perx(NULL);
	}

	while (1) {
		/* init pollfd structure */
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;

		printf("wait for event happening\n");
		/* start poll */
		n = poll(&pfd, 1, -1);
		printf("poll return %d\n", n);
		if (n) {
			pollhandle(fd);
		} else if (n < 0) {
			perx("poll");
		} else {
			/* n==0 will not happen */
			perx("poll:time out");
		}
	}

	close(fd);
	return 0;
}
