all: sr620

sr620: comSR620.cpp
	g++ -Wall -o sr620 comSR620.cpp -lrt

clean:
	rm -f sr620

sr620.exe : comSR620.c
	i686-w64-mingw32-gcc -static -Wall comSR620.cpp -o sr620.exe
