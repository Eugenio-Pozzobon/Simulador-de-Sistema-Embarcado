// Cliente - Simulador

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

SOCKET ConnectSocket = INVALID_SOCKET;
char recvbuf[DEFAULT_BUFLEN];
int iResult, sendResult, recvResult;
int recvbuflen = DEFAULT_BUFLEN;

int analogRead[8];
byte canRead[8];

FILE *fp, *fcan;

_Noreturn void *readAnalogicData() {
    while(true){
        for(int i = 0; i < 8 ; i++){
            analogRead[i] = rand() % 1024;
        }
    }
}

_Noreturn void *readCanData() {

}

_Noreturn void *sendTelemetryData() {

}

_Noreturn void *saveData() {
    while(true){ // inserir condição para interromper gravação e salvar dados
        for(int i = 0; i<8; i++){
            fprintf(fp, ",%d", canRead[i]);
        }
        for(int i = 0; i<8; i++){
            fprintf(fp, ",%d", analogRead[i]);
        }
    }
    fclose(fp);
}

_Noreturn void *sendBluetoothData() {

}

//
_Noreturn void *socketSend() {
    sendResult = 0;
    do {
        const char *sendbuf = "cmd0";
        sendResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
        if (sendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
        }

        printf("Bytes Sent: %d\n", sendResult);

    } while (sendResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
}

//
_Noreturn void *socketRecieve() {
    // Receive until the peer closes the connection
    while(true){

        recvResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( recvResult > 0 )
            printf("Bytes received: %s\n", recvbuf);
        else if ( recvResult == 0 )
            printf("Connection closed\n");
        else
            break;
            printf("recv failed with error: %d\n", WSAGetLastError());

    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
}

int main(int argc, char *argv[]) {

    WSADATA wsaData;

    struct addrinfo *result = NULL,
            *ptr = NULL,
            hints;
    const char *sendbuf = "this is a test";


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    fp = fopen ("data.txt", "a+");
    fcan = fopen ("can.txt", "r");

    pthread_t send_thread, recieve_thread, save_thread, can_thread, readAnalog_thread, telemetry_thread, bluetooth_thread;

    pthread_create(&send_thread, NULL, socketSend, NULL);
    pthread_create(&recieve_thread, NULL, socketRecieve, NULL);
    pthread_create(&save_thread, NULL, saveData, NULL);
    pthread_create(&can_thread, NULL, readCanData, NULL);
    pthread_create(&readAnalog_thread, NULL, readAnalogicData, NULL);
    pthread_create(&telemetry_thread, NULL, sendTelemetryData, NULL);
    pthread_create(&bluetooth_thread, NULL, sendBluetoothData, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(recieve_thread, NULL);
    pthread_join(save_thread, NULL);
    pthread_join(can_thread, NULL);
    pthread_join(readAnalog_thread, NULL);
    pthread_join(telemetry_thread, NULL);
    pthread_join(bluetooth_thread, NULL);

    //fclose(fp);

    return 0;
}