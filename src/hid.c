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
				case 'o':
					msgSet_gain(msgOption_gain + arg1);
					action1("increase gain by %d dB");
					break;
				case 'y':
					msgSet_gain(msgOption_gain - arg1);
					action1("decrease gain by %d dB");
					break;
				case 'O':
					msgSet_dynamicGain(!msgOption_dynamicGain);
					action("toggle dynamic gain");
					break;

				// change signal to noise ratio
				case 'l':
					msgSet_signalToNoise(msgOption_signalToNoise + arg1);
					action1("increase signal to noise ratio by %d dB");
					break;
				case 'h':
					msgSet_signalToNoise(msgOption_signalToNoise - arg1);
					action1("decrease signal to noise ratio by %d dB");
					break;

				// play/pause
				case ' ':
					if (playerPlaying) {
						msgSend_pause();
					} else {
						msgSend_play();
					}
					key="space";
					action("play/pause");
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
					msgSet_overtoneBlur(msgOption_overtoneBlur + arg1);
					action1("increase overtoneBlur by %d px");
					break;
				case 'z':
					msgSet_overtoneBlur(msgOption_overtoneBlur - arg1);
					action1("decrease overtoneBlur by %d px");
					break;
				case 's':
					msgSet_overtoneThreshold(msgOption_overtoneThreshold + 10*arg1);
					action1("increase overtoneThreshold by %d0 px");
					break;
				case 'x':
					msgSet_overtoneThreshold(msgOption_overtoneThreshold - 10*arg1);
					action1("decrease overtoneThreshold by %d0 px");
					break;
				case 'd':
					msgSet_overtoneRatio(msgOption_overtoneRatio + 10*arg1);
					action1("increase overtoneRatio by %d0 px");
					break;
				case 'c':
					msgSet_overtoneRatio(msgOption_overtoneRatio - 10*arg1);
					action1("decrease overtoneRatio by %d0 px");
					break;
				case 'f':
					msgSet_overtoneAddition(msgOption_overtoneAddition + 10*arg1);
					action1("increase overtoneAddition by %d0 px");
					break;
				case 'v':
					msgSet_overtoneAddition(msgOption_overtoneAddition - 10*arg1);
					action1("decrease overtoneAddition by %d0 px");
					break;


				int blur        = msgOption_overtoneBlur;
				float threshold = msgOption_overtoneThreshold;
				float ratio     = msgOption_overtoneRatio;
				float addition  = msgOption_overtoneAddition;
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

