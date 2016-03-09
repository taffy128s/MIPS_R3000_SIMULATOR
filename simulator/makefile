CC = gcc -Wall
SRCS = ./*.c
OBS = ./*.o

# The following line means that do SRC first.
single_cycle: SRC
	$(CC) -o $@ *.o

SRC: $(SRCS)
	$(CC) -c $(SRCS)

clean: $(OBS)
	rm *.o single_cycle