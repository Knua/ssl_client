all: ssl_server

ssl_server:
	g++ -Wall -o ssl_server ssl_server.cpp -L/usr/lib -lssl -lcrypto -pthread
	
clean:
	rm -rf *.o ssl_server
