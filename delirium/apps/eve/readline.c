#define BUFSIZE	256

static void	(*lineHook)(char *);
static char	lineBuf[BUFSIZE];
static char	*bufp;

void initReadline(void (*lineHandler)(char *)) {
	lineHook = lineHandler;
	bufp = lineBuf;
}

/* keyboard input event 
 */
void keyInput(char key) {
	switch (key) {
		case '\b':
			if (bufp != lineBuf)
				bufp--;
			break;
		case '\n':
			*bufp = 0;
			bufp = lineBuf;
			lineHook(bufp);
			break;
		default:
			*(bufp) = key;
			if (bufp < (lineBuf + BUFSIZE - 1))
				bufp++;
	}
}
