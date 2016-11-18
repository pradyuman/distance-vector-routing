CFLAGS = -g -Wall
CC = gcc

SRC = src/
TEST = test/

# change based on type of router to be built
# value can be either DISTVECTOR or PATHVECTOR
ROUTERMODE = DISTVECTOR

# if DEBUG is 1, debugging messages are printed
DEBUG = 0

# Check which OS
OS := $(shell uname)
ifeq ($(OS), SunOS)
	SOCKETLIB = -lsocket
endif

# Network Emulator port
NE = 9999

all : router

endian.o: $(SRC)ne.h $(SRC)endian.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -c $(SRC)endian.c

routingtable.o: $(SRC)ne.h $(SRC)routingtable.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -c $(SRC)routingtable.c

router: endian.o routingtable.o $(SRC)router.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) endian.o routingtable.o $(SRC)router.c -o router -lnsl $(SOCKETLIB)

unit-test: routingtable.o $(TEST)unit-test.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) routingtable.o $(TEST)unit-test.c -o unit-test -lnsl $(SOCKETLIB)

run:
	rm -f router$(R).log
	./router $(R) localhost $(NE) 900$(R) &
	sleep 1
	tail -f router$(R).log

runall:
	./router 0 localhost $(NE) 9000 &
	./router 1 localhost $(NE) 9001 &
	./router 2 localhost $(NE) 9002 &
	./router 3 localhost $(NE) 9003 &

run-ne:
	bin/ne $(NE) $(FILE)

clean:
	rm -f *.o
	rm -f router
	rm -f *log
