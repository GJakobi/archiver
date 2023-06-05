CFLAGS = -Wall -std=c99 -g

SRCDIR = ./src

OBJDIR = ./obj

CC = gcc

# arquivos-objeto
	objects = main.o
     
all: main.o 
	$(CC) -o vina++ $(OBJDIR)/main.o

# codifica.o: codifica.c
# 	$(CC) -c $(CFLAGS) codifica.c

main.o: $(SRCDIR)/main.c
	$(CC) -c $(CFLAGS) $(SRCDIR)/main.c -o $(OBJDIR)/main.o

clean:
	-rm -f ./obj/*.o vina++



