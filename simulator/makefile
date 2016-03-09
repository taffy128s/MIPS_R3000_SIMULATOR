CC = gcc -Wall
SRCS = ./*.c
OBS = ./*.o

single_cycle: SRC
	$(CC) -o $@ *.o

SRC: $(SRCS)
	$(CC) -c $(SRCS)

clean: $(OBS)
	rm *.o single_cycle