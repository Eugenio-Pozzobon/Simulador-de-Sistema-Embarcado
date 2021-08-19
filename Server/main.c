#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <pthread.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)

#define TOTAL 5

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

struct nodo {
    SOCKET newsockfd;
};

struct nodo nodo[5];

_Noreturn void *cliente(void *arg) {
    long cid = (long)arg;
    int i, n;
    char buffer[256];
    while (1) {
        bzero(buffer,sizeof(buffer));
        n = read(nodo[cid].newsockfd,buffer,50);
        printf("Recebeu: %s - %lu\n", buffer,strlen(buffer));
        if (n <= 0) {
            printf("Erro lendo do socket id %lu!\n", cid);
            close(nodo[cid].newsockfd);
            nodo[cid].newsockfd = -1;

            pthread_exit(NULL);
        }
        // MUTEX LOCK - GERAL
        pthread_mutex_lock(&m);
        for (i = 0;i < TOTAL; i++) {
            if ((i != cid) && (nodo[i].newsockfd != -1)) {
                n = write(nodo[i].newsockfd,buffer,50);
                if (n <= 0) {
                    printf("Erro escrevendo no socket id %i!\n", i);
                    //                    close(nodo[i].newsockfd);
                    //                    nodo[i].newsockfd = -1;
                }
            }
        }
        pthread_mutex_unlock(&m);
        // MUTEX UNLOCK - GERAL
    }
}

int main(int argc, char *argv[]) {
    WSADATA wsa;
    SOCKET s;

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int sockfd, portno;
    pthread_t t;
    long i;

    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
    }

    printf("Socket created.\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8888 );

    if (bind(s, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("Erro fazendo bind!\n");
        exit(WSAGetLastError());
    }
    listen(s,5);

    while (1) {
        printf("Looping");
        for (i = 0; i < TOTAL; i++) {
            if (nodo[i].newsockfd == -1) break;
        }
        nodo[i].newsockfd = accept(s,(struct sockaddr *) &cli_addr,&clilen);
        // MUTEX LOCK - GERAL
        pthread_mutex_lock(&m);
        if (nodo[i].newsockfd < 0) {
            printf("Erro no accept!\n");
            exit(1);
        }
        printf("Creating P thread");
        pthread_create(&t, NULL, cliente, (void *)i);
        pthread_mutex_unlock(&m);
        // MUTEX UNLOCK - GERAL
    }
    close(s);
    return 0;
}