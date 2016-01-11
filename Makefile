CFLAGS=-pthread -m64 -std=gnu99
LDFLAGS=-lGL -lglut -lm

spectrograph: obj/main.o obj/drawer.o obj/fft.o
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

run: spectrograph
	./spectrograph

obj/%.o: src/%.c
	mkdir obj -p
	gcc -c $< -o $@ $(CFLAGS)

obj/main.o: src/drawer.h src/fft.h
obj/fft.o: src/fft.h
obj/drawer.o: src/drawer.h

clean:
	rm -rf obj spectrograph
