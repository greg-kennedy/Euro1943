CFLAGS = -O2 -fomit-frame-pointer -funroll-loops -Wall -Wextra
#CFLAGS = -Wall -Wextra -O0 -g
LFLAGS = -lSDL_net -Wall -Wextra

all:
	g++ $(CFLAGS) -c oss.c
	g++ $(CFLAGS) -c main.cpp
	g++ -o ../server-linux oss.o main.o $(LFLAGS)
	strip ../server-linux

clean:
	rm -rf *.o ../server-linux
