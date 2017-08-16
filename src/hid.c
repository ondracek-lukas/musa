// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <GL/freeglut.h>
#include <math.h>
#include <stdio.h>
#include "consoleIn.h"
#include "messages.h"
#include "player.h"
#include "consoleOut.h"


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

#define action(desc) \
		consolePrintMsg("%s: "desc, key)
#define action0(desc) {\
	if (arg>0) \
		consolePrintMsg("%d%s: "desc, arg, key, arg); \
	else \
		consolePrintMsg("%s: "desc, key, arg); \
}
#define action1(desc) {\
	if (arg>0) \
		consolePrintMsg("%d%s: "desc, arg, key, arg1); \
	else \
		consolePrintMsg("%s: "desc, key, arg1); \
}
#define action1s(desc) {\
	if (arg>0) \
		consolePrintMsg("%d%s: "desc, arg, key, arg1, (arg>1 ? "s" : "")); \
	else \
		consolePrintMsg("%s: "desc, key, arg1, (arg>1 ? "s" : "")); \
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
		if ((c>='0') && (c<='9')) {
			arg = arg*10 + c-'0';
			if (arg>=10000) arg=0;
			if (arg>0) {
				consolePrintMsg("%d", arg);
			}
		} else {
			char *key=(char[]){c, '\0'};
			switch(c) {

				// open console
				case ':':
					consoleKeyPress(c);
					break;

				// change minFreq
				case 'k':
					msgSet_minFreq(msgOption_minFreq * pow(2, arg1/12.0));
					action1s("increase lowest tone by %d semitone%s");
					break;
				case 'K':
					msgSet_minFreq(msgOption_minFreq * pow(2, arg1));
					action1s("increase lowest tone by %d octave%s");
					break;
				case 'j':
					msgSet_minFreq(msgOption_minFreq / pow(2, arg1/12.0));
					action1s("decrease lowest tone by %d semitone%s");
					break;
				case 'J':
					action1s("decrease lowest tone by %d octave%s");
					msgSet_minFreq(msgOption_minFreq / pow(2, arg1));
					break;

				// change maxFreq
				case 'i':
					msgSet_maxFreq(msgOption_maxFreq * pow(2, arg1/12.0));
					action1s("increase highest tone by %d semitone%s");
					break;
				case 'I':
					msgSet_maxFreq(msgOption_maxFreq * pow(2, arg1));
					action1s("increase highest tone by %d octave%s");
					break;
				case 'u':
					msgSet_maxFreq(msgOption_maxFreq / pow(2, arg1/12.0));
					action1s("decrease highest tone by %d semitone%s");
					break;
				case 'U':
					msgSet_maxFreq(msgOption_maxFreq / pow(2, arg1));
					action1s("decrease highest tone by %d octave%s");
					break;

				// change gain
				case 't':
					msgSet_gain(msgOption_gain + arg1);
					action1("increase gain by %d dB");
					break;
				case 'g':
					msgSet_gain(msgOption_gain - arg1);
					action1("decrease gain by %d dB");
					break;
				case 'G':
					msgSet_dynamicGain(!msgOption_dynamicGain);
					action("toggle dynamic gain");
					break;

				// change signal to noise ratio
				case 'b':
					msgSet_signalToNoise(msgOption_signalToNoise + arg1);
					action1("increase signal to noise ratio by %d dB");
					break;
				case 'v':
					msgSet_signalToNoise(msgOption_signalToNoise - arg1);
					action1("decrease signal to noise ratio by %d dB");
					break;

				// open
				case 'o':
					consoleKeyPress(':');
					consoleKeyPress('o');
					consoleKeyPress('p');
					consoleKeyPress('e');
					consoleKeyPress('n');
					consoleKeyPress(' ');
					break;
				case 'O':
					msgSend_openDeviceDefault();
					break;

				// play/pause
				case ' ':
					key="space";
					if (playerPlaying) {
						msgSend_pause();
						action("pause");
					} else {
						msgSend_play();
						action("play");
					}
					break;

				// seek in stream
				case 'l':
					msgSend_seekRel(arg1);
					action1s("seek forward by %d second%s");
					break;
				case 'h':
					msgSend_seekRel(-arg1);
					action1s("seek backward by %d second%s");
					break;

				// quit
				case 'q':
					msgSend_quit();
					action("quit");
					break;

				// change overtone filter parameters
				case 'w':
					msgSet_filterOvertones(!msgOption_filterOvertones);
					action("toggle overtone filtering");
					break;
				case 'a':
					msgSet_overtoneThreshold(msgOption_overtoneThreshold + 10*arg1);
					action1("increase overtoneThreshold by %d0 px");
					break;
				case 'z':
					msgSet_overtoneThreshold(msgOption_overtoneThreshold - 10*arg1);
					action1("decrease overtoneThreshold by %d0 px");
					break;
				case 's':
					msgSet_overtoneRatio(msgOption_overtoneRatio + 10*arg1);
					action1("increase overtoneRatio by %d0 px");
					break;
				case 'x':
					msgSet_overtoneRatio(msgOption_overtoneRatio - 10*arg1);
					action1("decrease overtoneRatio by %d0 px");
					break;
				case 'd':
					msgSet_overtoneAddition(msgOption_overtoneAddition + 10*arg1);
					action1("increase overtoneAddition by %d0 px");
					break;
				case 'c':
					msgSet_overtoneAddition(msgOption_overtoneAddition - 10*arg1);
					action1("decrease overtoneAddition by %d0 px");
					break;
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
		char *key="";
		switch(c) {
			// seek in stream
			case GLUT_KEY_RIGHT:
				msgSend_seekRel(5);
				key = "right";
				action("Seek forward by 5 seconds.");
				break;
			case GLUT_KEY_LEFT:
				msgSend_seekRel(-5);
				key = "left";
				action("Seek backward by 5 seconds.");
				break;
			case GLUT_KEY_UP:
				msgSend_seekRel(60);
				key = "up";
				action("Seek forward by 1 minute.");
				break;
			case GLUT_KEY_DOWN:
				msgSend_seekRel(-60);
				key = "down";
				action("Seek backward by 1 minute.");
				break;
		}
		arg = 0;
	}
}

