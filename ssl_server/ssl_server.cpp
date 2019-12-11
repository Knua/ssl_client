#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <list> // for managing descriptor
#include <algorithm> // for find
#include <thread> // for thread
#include <mutex> // for lock

#define BUF_SIZE 1000
#define FAIL    -1

using namespace std;

bool b_opt_check = false;
void usage() {
    printf("syntax: ssl_server <port> [-b]\n");
    printf("sample: ssl_server 1234 -b\n");
}
mutex m;
list<SSL*> client_childfd;

// Create the SSL socket and intialize the socket address structure
int OpenListener(int port)
{
    int sd;
    struct sockaddr_in addr;
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        printf("can't bind port\n");
        abort();
    }
    if ( listen(sd, 10) != 0 )
    {
        printf("Can't configure listening port\n");
        abort();
    }
    return sd;
}
int isRoot()
{
    if (getuid() != 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
SSL_CTX* InitServerCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    method = (SSL_METHOD *) TLSv1_2_server_method();  /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    // else printf("No certificates.\n");
}
void Servlet(SSL* ssl) /* Serve the connection -- threadable */
{
    int sd, bytes;

    if ( SSL_accept(ssl) == FAIL )     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else{
        while (true) {
            if(find(client_childfd.begin(), client_childfd.end(), ssl) == client_childfd.end()) break;

            char buf[BUF_SIZE] = {0};
            ShowCerts(ssl);        /* get any certificates */
            bytes = SSL_read(ssl, buf, BUF_SIZE - 1); /* get request */
            if (bytes <= 0) {
                ERR_print_errors_fp(stderr);
                break;
            }
            buf[bytes] = '\0';
            printf("Received: \"%s\"\n", buf);

            if(b_opt_check){
                m.lock();
                for(auto it = client_childfd.begin(); it != client_childfd.end(); it++){
                    if(SSL_write(*it, buf, strlen(buf)) == 0){
                        client_childfd.erase(it);
                    }
                }
                m.unlock();
            }
            else {
                if (SSL_write(ssl, buf, strlen(buf)) <= 0) {
                    printf("send failed\n");
			        break;
                }
            }
        }
    }
	m.lock();
	client_childfd.erase(find(client_childfd.begin(), client_childfd.end(), ssl));
    m.unlock();

    sd = SSL_get_fd(ssl);       /* get socket connection */
    SSL_free(ssl);         /* release SSL state */
    close(sd);          /* close connection */
}

int main(int count, char * Argc[])
{
    SSL_CTX *ctx;
    char *portnum;

    //Only root user have the permsion to run the server
    if(!isRoot()){
        printf("This program must be run as root/sudo user!!");
        exit(0);
    }
    if (count != 2 && count != 3){
        usage();
        exit(0);
    }
	if(count == 3){
		if(strncmp(Argc[2], "-b", 2) == 0) b_opt_check = true;
		else{
			usage();
			exit(0);
		}
	}

    // Initialize the SSL library
    SSL_library_init();
    portnum = Argc[1];
    ctx = InitServerCTX();        /* initialize SSL */
    LoadCertificates(ctx, "test.com.pem", "test.com.pem"); /* load certs */
    int server = OpenListener(atoi(portnum));    /* create server socket */
    while (1)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL * ssl;
        int client = accept(server, (struct sockaddr*)&addr, &len);  /* accept connection as usual */
        if (client < 0) {
			printf("ERROR on accept");
			exit(0);
		}
        ssl = SSL_new(ctx);              /* get new SSL state with context */
        SSL_set_fd(ssl, client);      /* set connection socket to SSL state */

		m.lock();
		client_childfd.push_back(ssl);
		m.unlock();
        printf("Connection - %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        thread(Servlet, ssl).detach();         /* service connection */
    }
    close(server);          /* close server socket */
    SSL_CTX_free(ctx);         /* release context */
}

