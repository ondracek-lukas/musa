// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <GL/freeglut.h>
#include <math.h>
#include <stdio.h>
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

int arg = 0;
#define arg1 (arg?arg:1)

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
		if ((c>='0') && (c<='9')) {
			arg = arg*10 + c-'0';
			if (arg>=10000) arg=0;
			if (arg>0) {
				char str[10];
				sprintf(str, "%d", arg);
				msgSend_print(str);
			}
		} else {
			switch(c) {

				// open console
				case ':':
					consoleKeyPress(c);
					break;

				// change minFreq
				case 'k':
					msgSet_minFreq(msgOption_minFreq * pow(2, arg1/12.0));
					break;
				case 'K':
					msgSet_minFreq(msgOption_minFreq * pow(2, arg1));
					break;
				case 'j':
					msgSet_minFreq(msgOption_minFreq / pow(2, arg1/12.0));
					break;
				case 'J':
					msgSet_minFreq(msgOption_minFreq / pow(2, arg1));
					break;

				// change maxFreq
				case 'i':
					msgSet_maxFreq(msgOption_maxFreq * pow(2, arg1/12.0));
					break;
				case 'I':
					msgSet_maxFreq(msgOption_maxFreq * pow(2, arg1));
					break;
				case 'u':
					msgSet_maxFreq(msgOption_maxFreq / pow(2, arg1/12.0));
					break;
				case 'U':
					msgSet_maxFreq(msgOption_maxFreq / pow(2, arg1));
					break;

				// change gain
				case 'o':
					msgSet_gain(msgOption_gain + arg1);
					break;
				case 'y':
					msgSet_gain(msgOption_gain - arg1);
					break;
				case 'O':
					msgSet_dynamicGain(true);
					break;
				case 'Y':
					msgSet_dynamicGain(false);

				// change signal to noise ratio
				case 'l':
					msgSet_signalToNoise(msgOption_signalToNoise + arg1);
					break;
				case 'h':
					msgSet_signalToNoise(msgOption_signalToNoise - arg1);

			}
			arg = 0;
		}
	}
}


static void specialKeyPress(int c, int x, int y) {
	switch(c) {
		case 112: // left shift
		case 113: // right shift
			return;
	}
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
		arg = 0;
	}
}

