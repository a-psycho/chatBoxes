SRC=src/
INCL=-Iinclude/
LIB=-lpthread -lncurses
server,client:server.o client.o mysocket.o
	g++ mysocket.o server.o -o bin/server  $(LIB)
	g++ mysocket.o client.o -o bin/client $(LIB)
	rm *.o
%.o:$(SRC)%.cpp
	g++ -c $(SRC)*.cpp $(INCL)
