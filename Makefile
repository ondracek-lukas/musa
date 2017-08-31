# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

CFLAGS=-pthread -std=gnu99 -g
LDFLAGS=-lGL -lglut -lm -lavcodec -lavformat -lavutil -lswresample -lportaudio

musa: obj/main.o obj/util.o obj/drawer.o obj/player.o obj/playerFile.o obj/playerDevice.o obj/drawerBuffer.o obj/drawerMusicVisualiser.o obj/drawerScale.o obj/logFft.o obj/fft.o obj/taskManager.o obj/consoleIn.o obj/consoleOut.o obj/consoleStatus.o obj/hid.o obj/commandParser.o obj/messages.o obj/streamBuffer.o obj/resampler.o obj/overtoneFilter.o obj/resources.gen.o obj/mem.o
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

run: musa
	./musa

obj/%.o: src/%.c src/%.h
	mkdir obj -p
	gcc -c $< -o $@ $(CFLAGS)

messages=src/messagesList.gen.h src/messages.h

obj/main.o: src/drawer.h src/player.h src/util.h $(messages) src/commandParser.h src/hid.h src/resources.gen.h src/mem.h
obj/fft.o: src/mem.h
obj/logFft.o: src/fft.h src/resampler.h src/mem.h
obj/drawer.o: src/drawer*.h src/player.h $(messages) src/taskManager.h src/console*.h src/util.h src/resources.gen.h src/mem.h
obj/drawerScale.o: src/drawerBuffer.h src/player.h $(messages) src/mem.h
obj/drawerMusicVisualiser.o: src/drawerBuffer.h src/drawerScale.h src/player.h src/logFft.h src/taskManager.h src/overtoneFilter.h $(messages) src/resampler.h src/drawer.h src/mem.h
obj/taskManager.o: src/util.h src/mem.h

obj/hid.o: src/consoleIn.h $(messages) src/player.h src/consoleOut.h
obj/consoleIn.o: src/util.h src/commandParser.h $(messages) src/mem.h
obj/consoleOut.o: src/util.h $(messages) src/mem.h
obj/consoleStatus.o: src/util.h src/player.h src/messages.h src/drawerScale.h src/taskManager.h src/mem.h
obj/commandParser.o: $(messages) src/util.h src/mem.h
obj/messages.o: src/messagesList.gen.c $(messages) src/taskManager.h src/util.h src/mem.h
obj/resampler.o: src/streamBuffer.h src/player.h src/taskManager.h src/fft.h $(messages) src/drawer.h src/mem.h
obj/player.o: src/taskManager.h src/util.h $(messages) src/playerFile.h src/playerDevice.h src/consoleOut.h src/resources.gen.h src/mem.h
obj/playerFile.o: src/taskManager.h src/util.h $(messages) src/consoleOut.h src/player.h src/playerDevice.h src/mem.h
obj/playerDevice.o: src/taskManager.h src/util.h $(messages) src/consoleOut.h src/player.h $(messages) src/streamBuffer.h
obj/overtoneFilter.o: $(messages) src/drawerScale.h src/mem.h
obj/streamBuffer.o:
obj/util.o: src/mem.h

src/resources.gen.c: src/resourcesGen.pl VERSION logo.mp3 src/intro.txt src/help.txt src/help.txt README COPYING
	src/resourcesGen.pl VERSION logo.mp3 src/intro.txt src/help.txt README COPYING
src/resources.gen.h: src/resourcesGen.pl VERSION logo.mp3 src/intro.txt src/help.txt README COPYING
	src/resourcesGen.pl VERSION logo.mp3 src/intro.txt src/help.txt README COPYING

src/messagesList.gen.h: src/messagesList.pl src/messagesGen.pl
	src/messagesGen.pl
src/messagesList.gen.c: src/messagesList.pl src/messagesGen.pl src/*.h
	src/messagesGen.pl

clean:
	rm -rf obj musa src/*.gen.*
