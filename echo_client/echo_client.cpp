#include <stdio.h> // for perror
#include <stdlib.h> // for atoi
#include <string.h> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket

#include <thread>

using namespace std;
#define BUFSIZE 1024

void usage() {
    printf("syntax: echo_client <host> <port>\n");
    printf("sample: echo_client 127.0.0.1 1234\n");
}

void send_echo(int sockfd, char * buf){
	while(true){
		memset(buf, '\0', sizeof(buf));

		scanf("%s", buf);
		if (strcmp(buf, "quit") == 0) break;

		ssize_t sent = send(sockfd, buf, strlen(buf), 0);
		if (sent == 0) {
			perror("send failed");
			break;
		}
	}
}
void recv_echo(int sockfd, char * buf){
	while(true){
		memset(buf, '\0', sizeof(buf));

		ssize_t received = recv(sockfd, buf, BUFSIZE - 1, 0);
		if (received == 0 || received == -1) {
			perror("recv failed");
			break;
		}
		buf[received] = '\0'; // cut buffer overflow
		printf("%s\n", buf);
	}
}

int main(int argc, char * argv[]) 
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket failed");
		return -1;
	}
	if (argc != 3){
        usage();
        return -1;
    }
	int host;
    int port = atoi(argv[2]);
	if(inet_pton(AF_INET, argv[1], &host) != 1){
		perror("convert ip failed");
		return -1;
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = host;
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
	if (res == -1) {
		perror("connect failed");
		return -1;
	}
	printf("connected\n");

	char send_buf[BUFSIZE];
	char recv_buf[BUFSIZE];

	thread send_thr = thread(send_echo, sockfd, send_buf);
	thread recv_thr = thread(recv_echo, sockfd, recv_buf);

	send_thr.join();
	recv_thr.join();

	close(sockfd);
}
