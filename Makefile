# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

CFLAGS=-pthread -m64 -std=gnu99 -g
LDFLAGS=-lGL -lglut -lm

musa: obj/main.o obj/util.o obj/drawer.o obj/player.o obj/drawerBuffer.o obj/drawerMusicVisualiser.o obj/drawerScale.o obj/logFft.o obj/fft.o obj/taskManager.o obj/consoleIn.o obj/consoleOut.o obj/hid.o obj/commandParser.o obj/messages.o
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

run: musa
	./musa

obj/%.o: src/%.c
	mkdir obj -p
	gcc -c $< -o $@ $(CFLAGS)

messages=src/messagesList.gen.h src/messages.h

obj/main.o: src/main.h src/drawer.h src/player.h src/util.h $(messages) src/commandParser.h
obj/util.o: src/util.h
obj/fft.o: src/fft.h src/util.h
obj/logFft.o: src/logFft.h src/util.h src/fft.h
obj/player.o: src/player.h
obj/drawer.o: src/drawer*.h src/player.h $(messages) src/taskManager.h
obj/drawerBuffer.o: src/drawerBuffer.h
obj/drawerScale.o: src/drawerScale.h src/drawerBuffer.h src/util.h
obj/drawerMusicVisualiser.o: src/drawerMusicVisualiser.h src/drawerBuffer.h src/drawerScale.h src/player.h src/util.h src/logFft.h src/taskManager.h
obj/taskManager.o: src/taskManager.h src/util.h

obj/hid.o: src/hid.h src/consoleIn.h $(messages)
obj/consoleIn.o: src/consoleIn.h src/util.h src/commandParser.h src/messages.h
obj/consoleOut.o: src/consoleOut.h src/util.h
obj/commandParser.o: src/commandParser.h $(messages)
obj/messages.o: src/messagesList.gen.c $(messages) src/taskManager.h

src/messagesList.gen.h: src/messagesList.pl src/messagesGen.pl
	src/messagesGen.pl
src/messagesList.gen.c: src/messagesList.pl src/messagesGen.pl
	src/messagesGen.pl

clean:
	rm -rf obj musa src/messagesList.gen.*
