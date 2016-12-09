// MusA  Copyright (C) 2016  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

#include <stdlib.h>
#include <stdio.h>

void *utilMalloc(size_t size) {
	void *mem = malloc(size);
	if (!mem) {
		printf("Error: Out of memory (%d B needed)\n", size);
		exit(1);
	}
	return mem;
}
