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
        if (!readn(conn->sock, (char*)&msg_len, sizeof(msg_len))) {
            printf("admin: disconnect from '%s(%hu)'\n", clienthost, clientport);
            close(conn->sock);
            return;
        }

        /* message length is stored at msg_len */
        
        msg_len = ntohl(msg_len);

        msg = NULL;
		msg = (char *) malloc(1000);
		char* post;
		post =(char *) malloc(1000);
        /*if (!msg) {
            fprintf(stderr, "error: unable to malloc\n");
            return NULL;
        }
        if (!readn(conn->sock,msg,msg_len)) {
            free(msg); 
            return;
        }*/
		strcpy(msg, "GET http");
		read(conn->sock, post, 1000);
		strcat(msg, post);
		//printf("%s\n", msg);
		//printf("%s(%hu): %s", clienthost, clientport, msg);
		/* connect to http server*/
		int servSock;
		int i;
		int j;
		j=0;
		/*find the host info in the http msg*/
		char * hostStart = (strstr(msg, "Host:"));
		char * hostEnd = (strstr(hostStart, "\n"));
		int hostInfoLen=hostEnd-hostStart;
		hostInfoLen = hostInfoLen -7;
		char hostInfo[1000];
		strncpy(hostInfo, hostStart+6, hostInfoLen);
		
		//printf("%s\n", hostInfo);		
		/*move a reference to it to a new place in memory*/
		j=0;
		int altPort = 0;
		/*check to see if there is an alternate port being used*/
		for(i=-1; i>=0; i--){
			if(!isdigit(hostInfo[i])){
				if(hostInfo[i] == ':'){
					altPort = 1;
				}else{
					i = -1;
				}
			}else{
				j++;
			}
		}
		char port[1000];
		char hostName[1000];
		char msgResponse1[1000];
		char *msgResponse2;
		/*if there is we need to rename the host and set a new port*/
		if(altPort == 1){
			/*we need to remove the ":" from inbetween the host and port*/
			for(i=0; i<hostInfoLen-j; i++){
				if(i<hostInfoLen-j){
					hostName[i]=hostInfo[i];
				}else if(i> hostInfoLen-j){
					port[i-(hostInfoLen-(j+1))] = hostInfo[i];
				}
			}
		}else{
			port[0] = '8';
			port[1] = '0';
			strcpy(hostName, hostInfo);
		}
		//printf("%s\n%s\n",hostInfo, port);
		/* pass server message */
		servSock= hooktoserver(hostName, atoi(port));
		if (servSock == -1){
   			exit(1);
		}		
		write(servSock, msg,1000);
		/*ask for response*/
		read(servSock,msgResponse1,1000);
		//printf("%s\n",msgResponse1);
		char * conLenStart = (strstr(msgResponse1, "Content-Length:"));
		//printf("%s\n",conLenStart);
		char * conLenEnd = (strstr(conLenStart, "\n"));
		int conLenSize=conLenEnd-conLenStart;
		conLenSize = conLenSize -17;
		char conLenTxt[128];
		strncpy(conLenTxt, conLenStart+16, conLenSize);
		//printf("%s\n",conLenTxt);
		int conLen = atoi(conLenTxt)*12;
		conLen = conLen+1000;
		msgResponse2 =(char *) malloc(conLen);
		write(servSock, msg,conLen);
		read(servSock,msgResponse2,conLen);
		/*pass response to client*/
		write(conn->sock, msgResponse2, conLen);		
        //
		
        free(msg);
		free(msgResponse2);
		close(servSock);
		printf("admin: disconnect from server at '%s(%hu)'\n", hostName, atoi(port));
		close(conn->sock);
		printf("admin: disconnect from client.\n");
		return;
    
   
}
