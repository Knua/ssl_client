all: ssl_client

ssl_client:
	g++ -Wall -o ssl_client ssl_client.cpp -L/usr/lib -lssl -lcrypto -pthread

clean:
	rm -rf *.o ssl_client
