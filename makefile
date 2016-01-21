CC = gcc
CFLAGS = -c
OBJECTS = slist.o server.o

DEBUG_FLAGS = -g
DEBUG_OBJECTS = slist.c server.c

app: $(OBJECTS)
	$(CC) $(OBJECTS) -Wall -o server

debug: $(DEBUG_OBJECTS)
	$(CC) $(DEBUG_FLAGS) $(DEBUG_OBJECTS) -Wall -o server

clean:
	rm $(OBJECTS)
	rm server


server.o: server.c
	$(CC) $(CFLAGS) server.c

slist.o: slist.c slist.h
	$(CC) $(CFLAGS) slist.c
