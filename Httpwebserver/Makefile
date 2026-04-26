.PHONY 	= all clean

CC = gcc


BIN := server


all: clean
	${CC} server.c main.c -g -o ${BIN}


clean:
	@rm -rf ${BIN}