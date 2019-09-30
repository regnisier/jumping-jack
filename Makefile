CFLAGS+=-Wall -O0 -g3 -pthread -std=gnu99
LDLIBS=-lncurses -lpthread
EXECUTABLE=JumpingJack
all: $(EXECUTABLE)
clean:
	rm -rf *o $(EXECUTABLE)
