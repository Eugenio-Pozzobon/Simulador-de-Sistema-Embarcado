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

#define DEFAULT_BUFLEN 256
#define DEFAULT_PORT "8080"

pthread_mutex_t connectionMutex = PTHREAD_MUTEX_INITIALIZER;

int iResult, sendResult, recvResult;

char recvbuf[DEFAULT_BUFLEN];
char *sendbuf;

int recvbuflen = DEFAULT_BUFLEN;

bool sendCmd = false, exitProgram = false;

SOCKET ClientSocket = INVALID_SOCKET;

_Noreturn void *cmdMonitor() {
    char *cmd, *id_original, *id_final; //comando do usuário para o shell
    char entrada[DEFAULT_BUFLEN]; //entrada raw do usuario
    while(true){
        printf("cmd>");
        fgets(entrada, DEFAULT_BUFLEN, stdin); //pega o comando dado

        entrada[strlen(entrada)-1] = '\0'; //remove o '\n'

        cmd = strtok(entrada, " " ); //divide o comando para pegar só a parte que importa

        pthread_mutex_lock(&connectionMutex);
        memset(sendbuf, 0, DEFAULT_BUFLEN);
        strcpy(sendbuf, entrada);
        printf("\tsending cmd %s\n", sendbuf);
        pthread_mutex_unlock(&connectionMutex);

        if(strncmp(cmd, "sair" ,DEFAULT_BUFLEN) == 0){//comando sair do loop

            printf("\ncmd>\tEncerrando...\n");
            pthread_mutex_lock(&connectionMutex);
            exitProgram = true;
            pthread_mutex_unlock(&connectionMutex);
            break;

        }else if(strncmp(cmd, "help" ,DEFAULT_BUFLEN) == 0){
        }else{
            //identifica comando não encontrado
            printf("\n\tComando nao encontrado\n");
        }
    }
}

//
_Noreturn void *socketSend() {

    struct timespec ts = {0, 0};
    /* 0 and 1/10 seconds */
    ts.tv_sec  = 1;
    ts.tv_nsec = 000000000;

    while(true){
        //printf("\nLets Send...\n");
        pthread_mutex_lock(&connectionMutex);

        if(exitProgram){
            pthread_mutex_unlock(&connectionMutex);
            break;
        }

        if(sendCmd){
            //printf("\nSending...\n");
            sendResult = send( ClientSocket, sendbuf, (int)strlen(sendbuf), 0 );
            if (sendResult == SOCKET_ERROR) {
                printf("\nerror>send failed with error: %d\n", WSAGetLastError());
                break;
            }else{
            }
        }

        pthread_mutex_unlock(&connectionMutex);
        pthread_delay_np(&ts);
    }


    if(!exitProgram){
        printf("\ncmd>\tServer Closed\n");
        // shutdown the connection since we're done
        sendResult = shutdown(ClientSocket, SD_SEND);
        if (sendResult == SOCKET_ERROR) {
            printf("\nerror>shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
        }
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

}

//
_Noreturn void *socketRecieve() {
    // Receive until the peer closes the connection
    while(true){
        pthread_mutex_lock(&connectionMutex);

        if(exitProgram) {
            pthread_mutex_unlock(&connectionMutex);
            break;
        }

        if(!sendCmd){
            recvResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
            //printf("\nRecieving...\n");
            if ( recvResult > 0 ){
                sendCmd = true;
                //printf("Bytes received: %s\n", recvbuf);
            }else if ( recvResult == 0 ){
            }else{
                printf("error>recv failed with error: %d\n", WSAGetLastError());
                break;
            }
        }

        pthread_mutex_unlock(&connectionMutex);

    }

    printf("cmd>\tServer Closed\n");

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
        printf("error>WSAStartup failed with error: %d\n", iResult);
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
        printf("error>getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ClientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ClientSocket == INVALID_SOCKET) {
        printf("error>socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ClientSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("error>bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ClientSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("error>listen failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ClientSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("error>accept failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    printf("\nStarting Server\n");

    sendbuf = malloc(DEFAULT_BUFLEN);
    memset(sendbuf, 0, DEFAULT_BUFLEN);
    strcpy(sendbuf, "0");


    pthread_t send_thread, recieve_thread, cmd_thread;

    pthread_create(&recieve_thread, NULL, socketRecieve, NULL);
    pthread_create(&send_thread, NULL, socketSend, NULL);
    pthread_create(&cmd_thread, NULL, cmdMonitor, NULL);

    pthread_join(recieve_thread, NULL);
    pthread_join(send_thread, NULL);
    pthread_join(cmd_thread, NULL);

    return 0;
}