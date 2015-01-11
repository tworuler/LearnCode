Server.out: Server.o
	g++ Server.cpp -o Server.out -std=c++11

Server.o: Server.cpp
	g++ -c Server.cpp -o Server.o -std=c++11

.PHONY: clean
clean:
	rm -rf Server.o Server.out
