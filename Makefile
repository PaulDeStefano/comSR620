all: sr620

sr620: comSR620.c
	gcc -Wall -lrt comSR620.c -o sr620

clean:
	rm -f sr620

sr620.exe : comSR620.c
	i686-w64-mingw32-gcc -static -Wall comSR620.c -o sr620.exe
