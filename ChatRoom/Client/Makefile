Client.out: Client.o
	g++ Client.o -o Client.out -lpthread

Client.o:
	g++ -c Client.cpp -o Client.o -lpthread -std=c++11

.PHONY: clean
clean:
	rm -rf Client.o Client.out
