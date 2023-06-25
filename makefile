CFLAGS = -Wall -g

SRCDIR = ./src

OBJDIR = ./obj

CC = gcc

     
all: main.o files-list.o extract.o buffer.o archive-manipulation.o
	$(CC) -o vina++ $(OBJDIR)/*.o

main.o: $(SRCDIR)/main.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/main.c -o $(OBJDIR)/main.o

files-list.o: $(SRCDIR)/files-list/files-list.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/files-list/files-list.c -o $(OBJDIR)/files-list.o

extract.o: $(SRCDIR)/extract/extract.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/extract/extract.c -o $(OBJDIR)/extract.o

buffer.o: $(SRCDIR)/buffer/buffer.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/buffer/buffer.c -o $(OBJDIR)/buffer.o

archive-manipulation.o: $(SRCDIR)/archive-manipulation/archive-manipulation.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/archive-manipulation/archive-manipulation.c -o $(OBJDIR)/archive-manipulation.o

clean:
	-rm -f ./obj/*.o vina++



