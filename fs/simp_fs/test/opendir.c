#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

int main()
{
	DIR *dirp;	
	dirp = opendir("/simp");
	if (dirp == NULL) {
		perror("opendir");
		return -1;
	}

	return 0;
}
