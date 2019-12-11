#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <thread>

using namespace std;
#define BUF_SIZE 1024
#define FAIL    -1

void usage() {
    printf("syntax: echo_client <host> <port>\n");
    printf("sample: echo_client 127.0.0.1 1234\n");
}

void send_echo(SSL * ssl, char * buf){
	while(true){
		memset(buf, '\0', BUF_SIZE);

		scanf("%s", buf);
		if (strcmp(buf, "quit") == 0) break;

		ssize_t sent = SSL_write(ssl, buf, strlen(buf));
		if (sent <= 0) {
			printf("send failed\n");
			break;
		}
	}
}
void recv_echo(SSL * ssl, char * buf){
	while(true){
        memset(buf, '\0', BUF_SIZE);

		ssize_t received = SSL_read(ssl, buf, BUF_SIZE - 1);
		if (received <= 0) {
			printf("recv failed\n");
			break;
		}
		buf[received] = '\0'; // cut buffer overflow
		printf("Received: \"%s\"\n", buf);
	}
}

int OpenConnection(const char *hostname, int port)
{
    int sd;
    struct hostent *host;
    struct sockaddr_in addr;
    if ( (host = gethostbyname(hostname)) == NULL )
    {
        perror(hostname);
        abort();
    }
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);
    if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        close(sd);
        perror(hostname);
        abort();
    }
    return sd;
}
SSL_CTX* InitCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = (SSL_METHOD*)TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("\tSubject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("\tIssuer: %s\n\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    // else printf("Info: No client certificates configured.\n");
}
int main(int argc, char * strings[])
{
    SSL_CTX *ctx;
    int server;
    SSL *ssl;
    char *hostname, *portnum;
    
    if (argc != 3){
        usage();
        return -1;
    }

    SSL_library_init();
    hostname=strings[1];
    portnum=strings[2];
    ctx = InitCTX();
    server = OpenConnection(hostname, atoi(portnum));
    ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, server);    /* attach the socket descriptor */

    if ( SSL_connect(ssl) == FAIL )   /* perform the connection */
        ERR_print_errors_fp(stderr);
    else{
        printf("Connected with %s encryption\n\n", SSL_get_cipher(ssl));
        ShowCerts(ssl);        /* get any certs */


	    char send_buf[BUF_SIZE];
        char recv_buf[BUF_SIZE];

        thread send_thr = thread(send_echo, ssl, send_buf);
        thread recv_thr = thread(recv_echo, ssl, recv_buf);

        send_thr.join();
        recv_thr.join();

        SSL_free(ssl);        /* release connection state */
    }
    close(server);         /* close socket */
    SSL_CTX_free(ctx);        /* release context */
    return 0;
}
