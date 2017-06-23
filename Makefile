CFLAGS=-pthread -m64 -std=gnu99 -g
LDFLAGS=-lGL -lglut -lm

musa: obj/main.o obj/util.o obj/drawer.o obj/player.o obj/drawerBuffer.o obj/drawerMusicVisualiser.o obj/drawerScale.o obj/logFft.o obj/fft.o obj/taskManager.o
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

run: musa
	./musa

obj/%.o: src/%.c
	mkdir obj -p
	gcc -c $< -o $@ $(CFLAGS)

obj/main.o: src/drawer.h src/player.h src/util.h
obj/util.o: src/util.h
obj/fft.o: src/fft.h src/util.h
obj/logFft.o: src/logFft.h src/util.h src/fft.h
obj/player.o: src/player.h
obj/drawer.o: src/drawer*.h src/player.h
obj/drawerBuffer.o: src/drawerBuffer.h
obj/drawerScale.o: src/drawerScale.h src/drawerBuffer.h src/util.h
obj/drawerMusicVisualiser.o: src/drawerMusicVisualiser.h src/drawerBuffer.h src/drawerScale.h src/player.h src/util.h src/logFft.h src/taskManager.h
obj/taskManager.o: src/taskManager.h src/util.h

clean:
	rm -rf obj musa
