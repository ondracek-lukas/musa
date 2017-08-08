# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

CFLAGS=-pthread -m64 -std=gnu99 -g
LDFLAGS=-lGL -lglut -lm

musa: obj/main.o obj/util.o obj/drawer.o obj/player.o obj/drawerBuffer.o obj/drawerMusicVisualiser.o obj/drawerScale.o obj/logFft.o obj/fft.o obj/taskManager.o obj/consoleIn.o obj/consoleOut.o obj/consoleStatus.o obj/hid.o obj/commandParser.o obj/messages.o obj/streamBuffer.o obj/resampler.o
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

run: musa
	./musa

obj/%.o: src/%.c src/%.h
	mkdir obj -p
	gcc -c $< -o $@ $(CFLAGS)

messages=src/messagesList.gen.h src/messages.h

obj/main.o: src/drawer.h src/player.h src/util.h $(messages) src/commandParser.h
obj/fft.o: src/util.h
obj/logFft.o: src/util.h src/fft.h
obj/drawer.o: src/drawer*.h src/player.h $(messages) src/taskManager.h src/console*.h
obj/drawerScale.o: src/drawerBuffer.h src/util.h
obj/drawerMusicVisualiser.o: src/drawerBuffer.h src/drawerScale.h src/player.h src/util.h src/logFft.h src/taskManager.h
obj/taskManager.o: src/util.h

obj/hid.o: src/consoleIn.h $(messages)
obj/consoleIn.o: src/util.h src/commandParser.h src/messages.h
obj/consoleOut.o: src/util.h
obj/consoleStatus.o: src/util.h src/player.h src/messages.h src/drawerScale.h
obj/commandParser.o: $(messages)
obj/messages.o: src/messagesList.gen.c $(messages) src/taskManager.h
obj/resampler.o: src/streamBuffer.h src/player.h src/util.h src/taskManager.h

src/messagesList.gen.h: src/messagesList.pl src/messagesGen.pl
	src/messagesGen.pl
src/messagesList.gen.c: src/messagesList.pl src/messagesGen.pl
	src/messagesGen.pl

clean:
	rm -rf obj musa src/messagesList.gen.*
