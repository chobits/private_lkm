#include <stdio.h>
#include <string.h>

int main()
{
	char buf1[10];
	char buf2[10];
	int n1, n2;
	memset(buf1, 'x', 10);
	n1 = snprintf(buf1, 3, "ab");
	printf("n1=%d\n", n1);

	memset(buf1, 0x0, 10);
	n2 = snprintf(buf1, 3, "aaaaaaaa");
	printf("n2=%d\n", n2);
	return 0;

}
