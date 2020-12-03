OBJS	= server.o client.o
OBJ_CLIENT = client.o
OBJ_SERVER = server.o
SOURCE	= server.c client.c
HEADER	= struct.h
OUT		= exec
OUTS	= server #out server
OUTC	= client #out client
CC	 	= g++
FLAGS	= -g -c -Wall
LFLAGS	= -lpthread

all: $(OBJS)
	$(CC) -g $(OBJ_CLIENT) -o $(OUTC) $(LFLAGS)
	$(CC) -g $(OBJ_SERVER) -o $(OUTS) $(LFLAGS)

client: $(OBJ_CLIENT)
	$(CC) -g $(OBJ_CLIENT) -o $(OUTC) $(LFLAGS)

server: $(OBJ_SERVER)
	$(CC) -g $(OBJ_SERVER) -o $(OUTS) $(LFLAGS)

server.o: server.c
	$(CC) $(FLAGS) server.c 

client.o: client.c
	$(CC) $(FLAGS) client.c 


clean:
	rm -f $(OBJS) $(OBJ_CLIENT) $(OBJ_SERVER) $(OUT) $(OUTS) $(OUTC)