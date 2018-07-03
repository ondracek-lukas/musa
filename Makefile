# MusA  Copyright (C) 2016--2017  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

CFLAGS   = -pthread -std=gnu99 -g
LDFLAGS  = -lm -lavcodec -lavformat -lavutil -lswresample -lportaudio

ifeq ($(arch), win32)
	CC       = i686-w64-mingw32-gcc
	CFLAGS  += -mwindows -DGLUT_DISABLE_ATEXIT_HACK -mthreads
	LDFLAGS += -lfreeglut -lglu32 -lopengl32
	exeext   = .exe
	objext   = .obj
else
	LDFLAGS += -lglut -lGL
	exeext   =
	objext   = .o
endif


musa$(exeext): obj/main$(objext) obj/util$(objext) obj/drawer$(objext) obj/player$(objext) obj/playerFile$(objext) obj/playerDevice$(objext) obj/drawerBuffer$(objext) obj/drawerMusicVisualiser$(objext) obj/drawerScale$(objext) obj/logFft$(objext) obj/fft$(objext) obj/taskManager$(objext) obj/consoleIn$(objext) obj/consoleOut$(objext) obj/consoleStatus$(objext) obj/hid$(objext) obj/commandParser$(objext) obj/messages$(objext) obj/streamBuffer$(objext) obj/resampler$(objext) obj/overtoneFilter$(objext) obj/resources.gen$(objext) obj/mem$(objext)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

run: musa
	./musa

obj/%$(objext): src/%.c src/%.h
	mkdir obj -p
	$(CC) -c $< -o $@ $(CFLAGS)

messages=src/messagesList.gen.h src/messages.h

obj/main$(objext): src/drawer.h src/player.h src/util.h $(messages) src/commandParser.h src/hid.h src/resources.gen.h src/mem.h
obj/fft$(objext): src/mem.h
obj/logFft$(objext): src/fft.h src/resampler.h src/mem.h
obj/drawer$(objext): src/drawer*.h src/player.h $(messages) src/taskManager.h src/console*.h src/util.h src/resources.gen.h src/mem.h
obj/drawerScale$(objext): src/drawerBuffer.h src/player.h $(messages) src/mem.h
obj/drawerMusicVisualiser$(objext): src/drawerBuffer.h src/drawerScale.h src/player.h src/logFft.h src/taskManager.h src/overtoneFilter.h $(messages) src/resampler.h src/drawer.h src/mem.h
obj/taskManager$(objext): src/util.h src/mem.h

obj/hid$(objext): src/consoleIn.h $(messages) src/player.h src/consoleOut.h
obj/consoleIn$(objext): src/util.h src/commandParser.h $(messages) src/mem.h
obj/consoleOut$(objext): src/util.h $(messages) src/mem.h
obj/consoleStatus$(objext): src/util.h src/player.h src/messages.h src/drawerScale.h src/taskManager.h src/mem.h
obj/commandParser$(objext): $(messages) src/util.h src/mem.h
obj/messages$(objext): src/messagesList.gen.c $(messages) src/taskManager.h src/util.h src/mem.h
obj/resampler$(objext): src/streamBuffer.h src/player.h src/taskManager.h src/fft.h $(messages) src/drawer.h src/mem.h
obj/player$(objext): src/taskManager.h src/util.h $(messages) src/playerFile.h src/playerDevice.h src/consoleOut.h src/resources.gen.h src/mem.h
obj/playerFile$(objext): src/taskManager.h src/util.h $(messages) src/consoleOut.h src/player.h src/playerDevice.h src/mem.h
obj/playerDevice$(objext): src/taskManager.h src/util.h $(messages) src/consoleOut.h src/player.h $(messages) src/streamBuffer.h
obj/overtoneFilter$(objext): $(messages) src/drawerScale.h src/mem.h
obj/streamBuffer$(objext):
obj/util$(objext): src/mem.h

src/resources.gen.c: src/resourcesGen.pl VERSION logo.mp3 src/intro.txt src/help.txt src/help.txt README COPYING
	src/resourcesGen.pl VERSION logo.mp3 src/intro.txt src/help.txt README COPYING
src/resources.gen.h: src/resourcesGen.pl VERSION logo.mp3 src/intro.txt src/help.txt README COPYING
	src/resourcesGen.pl VERSION logo.mp3 src/intro.txt src/help.txt README COPYING

src/messagesList.gen.h: src/messagesList.pl src/messagesGen.pl
	src/messagesGen.pl
src/messagesList.gen.c: src/messagesList.pl src/messagesGen.pl src/*.h
	src/messagesGen.pl

clean:
	rm -rf obj musa musa.exe src/*.gen.*
