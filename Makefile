CC = gcc
CFLAGS = -Wall -ansi -pedantic -g -lm
MAIN = mush
OBJS = mush.o parseline.o
all : $(MAIN)

$(MAIN) : $(OBJS) mush.h parseline.h
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

parseline.o : parseline.c parseline.h
	$(CC) $(CFLAGS) -c parseline.c

mush.o : mush.c mush.h
	$(CC) $(CFLAGS) -c mush.c

clean: 
	rm *.o $(MAIN) 
