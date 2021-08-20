// Software - Monitor

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <pthread.h>
#include <stdbool.h>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

pthread_mutex_t sendMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t recvMutex = PTHREAD_MUTEX_INITIALIZER;

int iResult, sendResult, recvResult;

char recvbuf[DEFAULT_BUFLEN];
char *sendbuf = "this is a test";

int recvbuflen = DEFAULT_BUFLEN;

bool sendCmd = false;

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;


_Noreturn void *cmdMonitor() {
    char *cmd, *id_original, *id_final; //comando do usuário para o shell
    char entrada[100]; //entrada raw do usuario
    while(true){
        printf("cmdMonitor>");
        fgets(entrada, 100, stdin); //pega o comando dado

        entrada[strlen(entrada)-1] = '\0'; //remove o '\n'

        cmd = strtok(entrada, " " ); //divide o comando para pegar só a parte que importa


        if(strncmp(cmd, "changeHz" ,100) == 0){
        }else if(strncmp(cmd, "tele" ,100) == 0){
        }else if(strncmp(cmd, "changeHz" ,100) == 0){ //comando dir
//             = strtok(NULL, " " );
//             = strtok(NULL, " " );
            pthread_mutex_lock(&sendMutex);
            sendCmd = true;
            sendbuf = cmd;
            pthread_mutex_unlock(&sendMutex);
        }else if(strncmp(cmd, "sair" ,100) == 0){//comando sair do loop
            break;

        }else if(strncmp(cmd, "help" ,100) == 0){
        }else{
            //identifica comando não encontrado
            printf("Comando não encontrado\n");
        }
    }
}
//
_Noreturn void *socketSend() {
    do {
        pthread_mutex_lock(&sendMutex);
        *sendbuf = "cmd0";
        sendResult = send( ClientSocket, sendbuf, (int)strlen(sendbuf), 0 );
        if (sendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
        }
        pthread_mutex_unlock(&sendMutex);

        printf("Bytes Sent: %d\n", sendResult);

    } while (sendResult > 0);

    // shutdown the connection since we're done
    sendResult = shutdown(ClientSocket, SD_SEND);
    if (sendResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
}

//
_Noreturn void *socketRecieve() {
    // Receive until the peer closes the connection
    do {
        pthread_mutex_lock(&recvMutex);
        recvResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if ( recvResult > 0 )
            printf("Bytes received: %s\n", recvbuf);
        else if ( recvResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

        pthread_mutex_unlock(&recvMutex);

    } while( recvResult > 0 );

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
}

int main(int argc, char *argv[]) {

    WSADATA wsaData;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    // closesocket(ListenSocket);

    // Receive until the peer shuts down the connection

    pthread_t send_thread, recieve_thread, cmd_thread;

    pthread_create(&send_thread, NULL, socketSend, NULL);
    pthread_create(&recieve_thread, NULL, socketRecieve, NULL);
    pthread_create(&cmd_thread, NULL, cmdMonitor, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(recieve_thread, NULL);
    pthread_join(cmd_thread, NULL);

    return 0;
}