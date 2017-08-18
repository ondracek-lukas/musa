# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

CFLAGS=-pthread -m64 -std=gnu99 -g
LDFLAGS=-lGL -lglut -lm -lavcodec -lavformat -lavutil -lswresample -lportaudio

musa: obj/main.o obj/util.o obj/drawer.o obj/player.o obj/playerFile.o obj/playerDevice.o obj/drawerBuffer.o obj/drawerMusicVisualiser.o obj/drawerScale.o obj/logFft.o obj/fft.o obj/taskManager.o obj/consoleIn.o obj/consoleOut.o obj/consoleStatus.o obj/hid.o obj/commandParser.o obj/messages.o obj/streamBuffer.o obj/resampler.o obj/overtoneFilter.o
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

run: musa
	./musa

obj/%.o: src/%.c src/%.h
	mkdir obj -p
	gcc -c $< -o $@ $(CFLAGS)

messages=src/messagesList.gen.h src/messages.h

obj/main.o: src/drawer.h src/player.h src/util.h $(messages) src/commandParser.h src/hid.h
obj/fft.o: src/util.h
obj/logFft.o: src/util.h src/fft.h src/resampler.h
obj/drawer.o: src/drawer*.h src/player.h $(messages) src/taskManager.h src/console*.h src/util.h
obj/drawerScale.o: src/drawerBuffer.h src/util.h src/player.h $(messages)
obj/drawerMusicVisualiser.o: src/drawerBuffer.h src/drawerScale.h src/player.h src/util.h src/logFft.h src/taskManager.h src/overtoneFilter.h $(messages) src/resampler.h src/drawer.h
obj/taskManager.o: src/util.h

obj/hid.o: src/consoleIn.h $(messages) src/player.h src/consoleOut.h
obj/consoleIn.o: src/util.h src/commandParser.h $(messages)
obj/consoleOut.o: src/util.h $(messages)
obj/consoleStatus.o: src/util.h src/player.h src/messages.h src/drawerScale.h src/taskManager.h
obj/commandParser.o: $(messages) src/util.h
obj/messages.o: src/messagesList.gen.c $(messages) src/taskManager.h src/util.h
obj/resampler.o: src/streamBuffer.h src/player.h src/util.h src/taskManager.h src/fft.h $(messages) src/drawer.h
obj/player.o: src/taskManager.h src/util.h $(messages) src/playerFile.h src/playerDevice.h src/consoleOut.h
obj/playerFile.o: src/taskManager.h src/util.h $(messages) src/consoleOut.h src/player.h src/playerDevice.h
obj/playerDevice.o: src/taskManager.h src/util.h $(messages) src/consoleOut.h src/player.h $(messages) src/streamBuffer.h
obj/overtoneFilter.o: $(messages) src/drawerScale.h src/util.h
obj/streamBuffer.o:
obj/util.o:

src/messagesList.gen.h: src/messagesList.pl src/messagesGen.pl
	src/messagesGen.pl
src/messagesList.gen.c: src/messagesList.pl src/messagesGen.pl src/*.h
	src/messagesGen.pl

clean:
	rm -rf obj musa src/messagesList.gen.*
