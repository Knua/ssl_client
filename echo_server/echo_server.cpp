#include <stdio.h> // for perror
#include <stdlib.h> // for atoi
#include <string.h> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket

#include <list> // for managing descriptor
#include <algorithm> // for find
#include <thread> // for thread
#include <mutex> // for lock
#define BUF_SIZE 1000

using namespace std;

bool b_opt_check = false;
void usage() {
    printf("syntax: echo_server <port> [-b]\n");
    printf("sample: echo_server 1234 -b\n");
}

mutex m;
list<int> client_childfd;

void echo(int now_childfd){
	while (true) {
		if(find(client_childfd.begin(), client_childfd.end(), now_childfd) == client_childfd.end()) break;

		char buf[BUF_SIZE];
		ssize_t received = recv(now_childfd, buf, BUF_SIZE - 1, 0);
		if (received == 0 || received == -1) {
			perror("recv failed");
			break;
		}
		buf[received] = '\0';
		printf("%s\n", buf);

		if(b_opt_check){
			m.lock();
			bool error_chk = false;
			for(auto it = client_childfd.begin(); it != client_childfd.end(); it++){
				if(send(*it, buf, strlen(buf), 0) == 0){
					client_childfd.erase(it);
				}
			}
			m.unlock();
		}
		else if (send(now_childfd, buf, strlen(buf), 0) == 0) {
			perror("send failed");
			break;
		}
		else {}
	}
	m.lock();
	client_childfd.erase(find(client_childfd.begin(), client_childfd.end(), now_childfd));
	m.unlock();
}

int main(int argc, char  * argv[]) 
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket failed");
		return -1;
	}
	if (argc != 2 && argc != 3){
        usage();
        return -1;
    }
	if(argc == 3){
		if(strncmp(argv[2], "-b", 2) == 0) b_opt_check = true;
		else{
			usage();
			return -1;
		}
	}
    int port = atoi(argv[1]);
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,  &optval , sizeof(int));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
	if (res == -1) {
		perror("bind failed");
		return -1;
	}

	res = listen(sockfd, 2); // backlog
	if (res == -1) {
		perror("listen failed");
		return -1;
	}

	while(true) {
		struct sockaddr_in new_addr;
		socklen_t clientlen = sizeof(sockaddr);
		
		int childfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&new_addr), &clientlen);
		if (childfd < 0) {
			perror("ERROR on accept");
			return -1;
		}
		m.lock();
		client_childfd.push_back(childfd);
		m.unlock();

		printf("connected\n");
		thread(echo, childfd).detach();
	}
	close(sockfd);
}