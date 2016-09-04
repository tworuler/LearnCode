Server.out: Server.o
	g++ Server.cpp -o Server.out -lmysqlclient -std=c++11

Server.o: Server.cpp
	g++ -c Server.cpp -o Server.o -lmysqlclient -std=c++11

.PHONY: clean
clean:
	rm -rf Server.o Server.out
