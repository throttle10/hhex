CC=gcc

all: hand_manager.o hhex_test.c
	$(CC) $? -o hhex_test 

hand_manager.o: hand_manager.c
	$(CC) -c $?

clean:
	rm -f *.o hhex_test
