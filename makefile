CC = gcc -Wall
SRCS = ./*.c

main: SRC
	$(CC) -o $@ *.o

SRC: $(SRCS)
	$(CC) -c $(SRCS)