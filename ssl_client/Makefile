all: ssl_client

echo_client:
	g++ -Wall -o ssl_client  ssl_client.c -L/usr/lib -lssl -lcrypto

clean:
	rm -rf *.o ssl_client
