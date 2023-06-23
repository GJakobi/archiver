CFLAGS = -Wall -std=c99 -g

SRCDIR = ./src

OBJDIR = ./obj

CC = gcc

     
all: main.o files-list.o
	$(CC) -o vina++ $(OBJDIR)/*.o

main.o: $(SRCDIR)/main.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/main.c -o $(OBJDIR)/main.o

files-list.o: $(SRCDIR)/files-list/files-list.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/files-list/files-list.c -o $(OBJDIR)/files-list.o

clean:
	-rm -f ./obj/*.o vina++



