all: main.c lock.c lock.h
	gcc `pkg-config --cflags --libs glib-2.0` -o lock main.c lock.c -g -Wall -Wextra -lpthread

debug: 
	gcc `pkg-config --cflags --libs glib-2.0` -o lock main.c lock.c -g -Wall -Wextra -fmudflapth -lmudflapth -lpthread

leaks: all
	G_SLICE=always-malloc valgrind --leak-check=full --show-reachable=yes ./lock

