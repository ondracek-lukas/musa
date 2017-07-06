// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>

#ifndef CONSOLE_SHARED_H
#define CONSOLE_SHARED_H

enum consoleSpecialChars { // Special: 3,4,5,6,7,\b; Free: 9
	consoleSpecialBack='\b',
	consoleSpecialColorNormal=3,
	consoleSpecialColorRed=4,
	consoleSpecialColorGreen=5,
	consoleSpecialColorBlue=6,
	consoleSpecialColorGray=7
};

// Returns the length of the longest line of str
// consoleSpecialChars are reflected
int consoleStrWidth(char *str);

// Returns number of lines of str
int consoleStrHeight(char *str);

#endif
