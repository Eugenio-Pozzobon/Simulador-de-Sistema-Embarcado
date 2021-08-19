
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

SOCKET s;

#define bzero(b, len) (memset((b), '\0', (len)), (void) 0)

int inet_aton(char *addr, struct in_addr *dest) {
    int a[4];
    if (sscanf(addr, "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3]) != 4)
        return 0;
    dest->s_addr = a[3] | a[2] << 8 | a[1] << 16 | a[0] << 24;
    return 1;
}

void *leitura(void *arg) {
    char buffer[256];
    int n;
    while (1) {
        bzero(buffer, sizeof(buffer));
        n = recv(s, buffer, 50, 0);
        if (n <= 0) {
            printf("Erro lendo do socket!\n");
            exit(1);
        }
        printf("MSG: %s", buffer);
    }
}

int main(int argc, char *argv[]) {
    int portno, n;
    struct sockaddr_in serv_addr;
    pthread_t t;

    WSADATA wsa;

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    //Create a socket
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
    }

    printf("Socket created.\n");

    char buffer[256];

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8888);

    if( bind(s ,(struct sockaddr *)&serv_addr , sizeof(serv_addr)) == SOCKET_ERROR){
	    printf("Erro conectando!\n");
        return -1;
    }

    printf("Conectado!\n");


    pthread_create(&t, NULL, leitura, NULL);
    do {
        bzero(buffer, sizeof(buffer));
        printf("Digite a mensagem (ou sair):");
        fgets(buffer, 50, stdin);
        n = send(s, buffer, 50, 0);
        if (n == -1) {
            printf("Erro escrevendo no socket!\n");
            return -1;
        }
        if (strcmp(buffer, "sair\n") == 0) {
            break;
        }
    } while (1);
    close(s);
    return 0;
}