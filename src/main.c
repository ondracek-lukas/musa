// MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#define _GNU_SOURCE
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "util.h"
#include "mem.h"
#include <unistd.h>

#include "player.h"
#include "drawer.h"
#include "hid.h"
#include "messages.h"
#include "commandParser.h"
#include "resources.gen.h"


void mainExit() {
	glutLeaveMainLoop();
}

int main(int argc, char **argv){
	char c;
	bool printHelp    = false;
	bool printReadme  = false;
	bool printCopying = false;
	bool openDevice   = false;
	double deviceFreq = 0;
	char *filename    = NULL;

	while (!printHelp && ((c=getopt(argc, argv, "?hrcd::-:")) != -1)) {
		switch (c) {
			case '?':
			case 'h':
				printHelp=true;
				break;
			case 'r':
				printReadme=true;
				break;
			case 'c':
				printCopying=true;
				break;
			case 'd':
				openDevice=true;
				if (optarg) {
					if (sscanf(optarg, "%lf", &deviceFreq) != 1) {
						printHelp = true;
					}
				}
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
	if (optind < argc) {
		if (optind == argc-1) {
			filename = argv[optind];
		} else {
			printHelp = true;
		}
	}

	if (filename && openDevice) {
		printHelp = true;
	}

	if (printHelp) {
		int versionCnt = 0;
		for (const char *strH = resources_help_txt; *strH; strH++) {
			if (*strH == 'V'-'@') versionCnt++;
		}
		char *str = NULL;
		memStrRealloc(&str, NULL,
				sizeof(resources_help_txt) + versionCnt * sizeof(resources_VERSION));
		char *str2 = str;
		for (const char *strH = resources_help_txt; *strH; strH++) {
			if (*strH == 'V'-'@') {
				for (const char *str2V = resources_VERSION; *str2V; str2V++) {
					*(str2++) = *str2V;
				}
			} else {
				*(str2++) = *strH;
			}
		}
		*str2 = '\0';
		printf("\n%s\n", str);
		exit(0);
	}
	if (printReadme) {
		printf("\n%s\n", resources_README);
		exit(0);
	}
	if (printCopying) {
		printf("\n%s\n", resources_COPYING);
		exit(0);
	}


	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE);
	glutInitWindowSize(1024, 768);
	glutCreateWindow("MusA");

	drawerInit();
	hidInit();

	if (filename) {
		msgSend_open(filename);
	} else if (openDevice) {
		if (deviceFreq) {
			playerOpenDevice(deviceFreq);
		} else {
			playerOpenDeviceDefault();
		}
	} else {
		playerOpenLogo();
	}

	glutMainLoop();
	return 0;
}

