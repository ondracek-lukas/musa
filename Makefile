CFLAGS=-pthread -m64 -std=gnu99 -g
LDFLAGS=-lGL -lglut -lm

musa: obj/main.o obj/drawer.o obj/player.o obj/drawerBuffer.o obj/drawerMusicVisualiser.o obj/drawerScale.o obj/logFft.o obj/fft.o
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

run: musa
	./musa

obj/%.o: src/%.c
	mkdir obj -p
	gcc -c $< -o $@ $(CFLAGS)

obj/main.o: src/drawer.h src/fft.h
obj/fft.o: src/fft.h
obj/player.o: src/player.h
obj/drawer.o: src/drawer*.h src/player.h
obj/drawerBuffer.o: src/drawerBuffer.h
obj/drawerScale.o: src/drawerScale.h
obj/drawerMusicVisualiser.o: src/drawerMusicVisualiser.h src/drawerBuffer.h

clean:
	rm -rf obj musa
