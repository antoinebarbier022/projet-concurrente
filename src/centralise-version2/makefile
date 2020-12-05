OBJS	= server.o client.o fonctions.o
OBJ_CLIENT = client.o fonctions.o
OBJ_SERVER = server.o fonctions.o
SOURCE	= server.c client.c fonctions.c
HEADER	= struct.h fonctions.h
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