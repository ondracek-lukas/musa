// MusA               Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file
// Geometric Figures  Copyright (C) 2015--2016  Lukáš Ondráček <ondracek.lukas@gmail.com>

// commandParser executes string commands defined in message module.

// The module is thread-safe.

#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdbool.h>


// Executes command using messages module.
bool cpExecute(char *cmd);

// Returns possible completions to given command prefix.
extern struct utilStrList *cpComplete(char *prefix);


// Returns possible completions to given path.
extern struct utilStrList *cpPathComplete(char *prefix);


// Returns possible completions to given color (+ mask #[AA]RRGGBB).
//extern struct utilStrList *consoleTranslColorComplete(char *prefix, bool withAlphaChannel);

#endif
