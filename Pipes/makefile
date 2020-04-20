#
# In order to execute this "Makefile" just type "make"
#	Muhammed Okumuş
#

OBJS	= main.o parser.o
SOURCE	= main.c parser.c
HEADER	= parser.h globals.h
OUT	= pipes
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 = -lm
# -g option enables debugging mode 
# -c flag generates object code for separate files


all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)


# create/compile the individual files >>separately<<
main.o: main.c
	$(CC) $(FLAGS) main.c 

parser.o: parser.c
	$(CC) $(FLAGS) parser.c 


# clean house
clean:
	rm -f $(OBJS) $(OUT)