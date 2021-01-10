TARGET = Asst3
CC = gcc
CFLAGS = -Wall

all : $(TARGET).c
	$(CC)  $(TARGET).c $(CFLAGS) -o KKJserver

clean: 
	rm -rf KKJserver *.o *.exe	
