all: ssl_server

ssl_server:
	g++ -Wall -o ssl_server ssl_server.c -L/usr/lib -lssl -lcrypto

clean:
	rm -rf *.o ssl_server
