/*--------------------------------------------------------------------*/
/* conference server */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

#include <stdlib.h>
#include <pthread.h>

extern int     startserver();

typedef struct 
{
    int sock;
    struct sockaddr_in sin_cli;
    socklen_t len;
} sock_connection_t;


void* process(void*);

int main(int argc, char* argv[])
{
    int servsock;
    pthread_t thread;
    sock_connection_t * connection;

    /* check usage */
    if (argc != 1) {
        fprintf(stderr, "usage : %s\n", argv[0]);
        exit(1);
    }

    /* get ready to receive requests */
    servsock = startserver();
    if (servsock == -1) {
        perror("Error on starting server: ");
        exit(1);
    }

    while (1) {
        connection = (sock_connection_t *)malloc(sizeof(sock_connection_t));

        connection->sock = accept(servsock, (struct sockaddr*)&connection->sin_cli, &connection->len);
        if (connection->sock<0) {
            free(connection);
            continue;
        }
        pthread_create(&thread,0, process, (void *)connection);
        pthread_detach(thread);
    }
}

void* process(void* ptr)
{
    sock_connection_t * conn;

    if (!ptr) pthread_exit(0);
    conn = (sock_connection_t*)ptr;

    struct hostent *hp;
    int cli_sd;

    char * clienthost;
    ushort clientport;

    if( getpeername(conn->sock, (struct sockaddr *)&conn->sin_cli, &conn->len) < 0 ){
        perror("Get peername error: ");
        exit(1);
    }
    clientport = ntohs(conn->sin_cli.sin_port);
    hp = gethostbyaddr(&conn->sin_cli.sin_addr, sizeof(conn->sin_cli.sin_addr), AF_INET);
    if(!hp){
        perror("Get hostname error: ");
        exit(1);
    }
    clienthost = hp->h_name;
    printf("admin: connect from '%s' at '%hu'\n", clienthost, clientport);

    char * msg;
    long msg_len;

    while (1) {
        if (!readn(conn->sock, (char*)&msg_len, sizeof(msg_len))) {
            printf("admin: disconnect from '%s(%hu)'\n", clienthost, clientport);
            close(conn->sock);
            break;
        }

        /* message length is stored at msg_len */
        
        msg_len = ntohl(msg_len);

        msg = NULL;
        msg = (char *) malloc(msg_len);
        if (!msg) {
            fprintf(stderr, "error: unable to malloc\n");
            return NULL;
        }
        if (!readn(conn->sock,msg,msg_len)) {
            free(msg); 
            continue;
        }

        printf("%s(%hu): %s", clienthost, clientport, msg);
        free(msg);
    
    }
}
