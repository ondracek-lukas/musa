// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#define _GNU_SOURCE
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "util.h"
#include <unistd.h>

#include "player.h"
#include "drawer.h"
#include "hid.h"
#include "messages.h"
#include "commandParser.h"


void mainExit() {
	glutLeaveMainLoop();
}

int main(int argc, char **argv){
	double sampleRate=0;

	char c;
	bool showHelp=false;
	while (!showHelp && ((c=getopt(argc, argv, "?i:r:c:u:a:-:")) != -1)) {
		switch (c) {
			case '?':
				showHelp=true;
				break;
			case 'i':
				sampleRate=strtof(optarg, &optarg);
				if (*optarg) showHelp=true;
				break;
			case '-':
				{
					char *str = NULL;
					asprintf(&str, "set %s", optarg);
					bool ret = cpExecute(str);
					free(str);
					if (!ret) {
						utilExitErr("Invalid option --%s", optarg);
					}
				}
				break;
		}
	}
	if (showHelp) {
		printf(
			"\n"
			"Help will be written later.\n" // XXX
			"\n\n"
			"This program is free software: you can redistribute it and/or modify\n"
			"it under the terms of the GNU General Public License version 3\n"
			"as published by the Free Software Foundation.\n"
			"\n"
			"This program is distributed in the hope that it will be useful,\n"
			"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			"GNU General Public License for more details.\n"
			"\n"
			"You should have received a copy of the GNU General Public License\n"
			"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
			"\n"
			"Copyright (C) 2015  Lukáš Ondráček <ondracek.lukas@gmail.com>.\n"
			"\n");
		exit(0);
	}

	// check values
	if (sampleRate<0) {
		printf("Wrong sampling rate.\n");
		exit(1);
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE);
	glutCreateWindow("MusA (musical analyser)");

	playerUseFDQuiet(stdin, sampleRate);
	drawerInit();
	hidInit();

	glutMainLoop();
	return 0;
}

