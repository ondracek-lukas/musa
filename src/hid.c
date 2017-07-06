// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <GL/freeglut.h>
#include "consoleIn.h"
#include "messages.h"


#define KEY_ENTER 13
#define KEY_ESC   27
#define KEY_BS     8
#define KEY_DEL  127

static void keyPress(unsigned char c, int x, int y);
static void specialKeyPress(int c, int x, int y);

void hidInit() {
	glutKeyboardFunc(keyPress);
	glutSpecialFunc(specialKeyPress);
}

static void keyPress(unsigned char c, int x, int y) {
	if (consoleIsOpen()) {
		switch(c) {
			case KEY_ENTER:
				consoleEnter();
				break;
			case KEY_ESC:
				consoleEsc();
				msgSend_consoleClear();
				break;
			case KEY_BS:
				consoleBackspace();
				break;
			case KEY_DEL:
				consoleDelete();
				break;
			case '\t':
				consoleTab();
				break;
			default:
				if ((c>=32) && (c<127)) // printable chars + space
					consoleKeyPress(c);
				break;
		}
	} else {
		msgSend_consoleClear();
		switch(c) {
			case ':':
				consoleKeyPress(c);
				break;
		}
	}
}


static void specialKeyPress(int c, int x, int y) {
	if (consoleIsOpen()) {
		switch(c) {
			case GLUT_KEY_UP:
				consoleUp();
				break;
			case GLUT_KEY_DOWN:
				consoleDown();
				break;
			case GLUT_KEY_LEFT:
				consoleLeft();
				break;
			case GLUT_KEY_RIGHT:
				consoleRight();
				break;
			case GLUT_KEY_HOME:
				consoleHome();
				break;
			case GLUT_KEY_END:
				consoleEnd();
				break;
		}
	} else {
		msgSend_consoleClear();
	}
}

