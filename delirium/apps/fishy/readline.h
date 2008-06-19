#ifndef __READLINE_H
#define __READLINE_H

void initReadline(void (*lineHandler)(char *));
void keyInput(int key);

#endif
