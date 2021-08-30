// Cliente - Simulador

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8080"


// Velocidade das threads
#define CAN_HZ  5
#define BT_HZ   3
#define TELE_HZ 1
#define AN_HZ   5

pthread_mutex_t connectionMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t canMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t analogMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t savingDataMutex = PTHREAD_MUTEX_INITIALIZER;

SOCKET ConnectSocket = INVALID_SOCKET;
char recvbuf[DEFAULT_BUFLEN];
int iResult, sendResult, recvResult;
int recvbuflen = DEFAULT_BUFLEN;

int newHz = 10;

//constantes sobre os dados a serem lidos
int analogReadRaw[6];
float analogRead[6];
float min_an[6] = {-4, -4, -4, -90, -180, 0};
float max_an[6] = {4, 4, 4, 90, 180, 260};

//can frame
byte canRead[8];

//  bluetooth var
float rpm = 0;
float gear = 0;
float eng_temp = 0;
float oil_pres = 0;

const char *okbuf = "cmdok";

// sinaliza quando o sistema pode enviar algo para o servidor
bool sendCmd = true;

//arquivos a serem escritos/lidos
FILE *fp, *fcan;

//conversão de dados analógicos
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//leitura de dados
_Noreturn void *readAnalogicData() {

    clock_t begin = clock();

    while (true) {

        double time = (double) (clock() - begin) / CLOCKS_PER_SEC;

        if (time * AN_HZ > 1) {// analog Hz
            begin = clock();
            pthread_mutex_lock(&analogMutex);

            //lê as portas analógicas
            for (int i = 0; i < 6; i++) {
                analogReadRaw[i] = rand() % 1024;
            }

            // converte os valores da leitura analógica
            for (int i = 0; i < 6; i++) {
                analogReadRaw[i] = mapFloat(analogReadRaw[i], 0, 1023, min_an[i], max_an[i]);
            }
            pthread_mutex_unlock(&analogMutex);
        }
    }
}

// lê a rede CAN (simulado com um arquivo de dados CAN que precisa ser lido)
_Noreturn void *readCanData() {

    fcan = fopen("can.csv", "r");

    clock_t begin = clock();

    while (true) {
        double time = (double) (clock() - begin) / CLOCKS_PER_SEC;

        if (time * CAN_HZ > 1) {// CAN Hz
            //todo: parse CAN file
            begin = clock();
            rpm = 0;
            gear = 0;
            eng_temp = 0;
            oil_pres = 0;
        }
    }
}

// imprime dados de telemetria
_Noreturn void *sendTelemetryData() {

    clock_t begin = clock();

    while (true) {
        double time = (double) (clock() - begin) / CLOCKS_PER_SEC;

        if (time * TELE_HZ > 1) {// Lora Hz
            begin = clock();

            //acessa ambos mutex juntos para não fracionar o print
            pthread_mutex_lock(&canMutex);
            pthread_mutex_lock(&analogMutex);
            printf("\ttele>");
            for (int i = 0; i < 8; i++) {
                printf("%d,", canRead[i]);
            }
            for (int i = 0; i < 6; i++) {
                printf("%d,", analogRead[i]);
            }
            printf("\n");
            pthread_mutex_unlock(&analogMutex);
            pthread_mutex_unlock(&canMutex);
        }

    }
}


// salva dados periodicamente na pasta de arquivos para armazenar o log
_Noreturn void *saveData() {
    clock_t begin = clock();

    while (true) {

        pthread_mutex_lock(&savingDataMutex);
        double time = (double) (clock() - begin) / CLOCKS_PER_SEC;
        double elapsed = time * newHz;
        pthread_mutex_unlock(&savingDataMutex);

        if (elapsed > 1) {
            begin = clock(); //algoritmo de compressão pela diferença de valores

            fp = fopen("data.txt", "a+");
            fprintf(fp, "%f", time);

            pthread_mutex_lock(&canMutex);
            for (int i = 0; i < 8; i++) {
                fprintf(fp, "%d,", canRead[i]);
            }
            pthread_mutex_unlock(&canMutex);

            pthread_mutex_lock(&analogMutex);
            for (int i = 0; i < 6; i++) {
                fprintf(fp, "%f,", analogRead[i]);
            }
            pthread_mutex_unlock(&analogMutex);
            fprintf(fp, "\n");
            fclose(fp);
        }
    }
}

// send bluetooth data to de pilot monitor
_Noreturn void *sendBluetoothData() {
    clock_t begin = clock();

    while (true) {
        double time = (double) (clock() - begin) / CLOCKS_PER_SEC;

        if (time * BT_HZ > 1) {//Bluetooth Hz
            begin = clock();

            pthread_mutex_lock(&canMutex);
            printf("\tpilot>");
            byte rpm_state = 0;
            rpm_state = rpm > 200 ? 1 : 0;
            rpm_state = rpm > 1500 ? 3 : 0;
            rpm_state = rpm > 3500 ? 2 : 0;
            rpm_state = rpm > 5500 ? 4 : 0;
            rpm_state = rpm > 7500 ? 5 : 0;
            rpm_state = rpm > 9500 ? 6 : 0;
            rpm_state = rpm > 10500 ? 7 : 0;

            //envia somente a parte inteira
            printf("\tRPM: %d", rpm_state);
            printf("\tG: %0.f", gear);
            printf("\tET: %0.f", eng_temp);
            printf("\tOP: %0.f", oil_pres);
            printf("\n");
            pthread_mutex_unlock(&canMutex);
        }

    }
}

//
void *socketSend() {
    sendResult = 0;
    const char *sendbuf = "0";

    clock_t begin = clock();

    while (true) {

        double time = (double) (clock() - begin) / CLOCKS_PER_SEC;

        if (time > 0.1) {
            begin = clock();
            pthread_mutex_lock(&connectionMutex);

            if (sendCmd) {
                //printf("\nSending...\n");
                sendResult = send(ConnectSocket, sendbuf, (int) strlen(sendbuf), 0);
                if (sendResult == SOCKET_ERROR) {
                    //printf("send failed with error: %d\n", WSAGetLastError());
                    break;
                } else {
                    sendCmd = false;
                }
                //printf("Bytes Sent: %d\n", sendResult);
            }

            pthread_mutex_unlock(&connectionMutex);
        }
    }

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
void *socketRecieve() {
    // Receive until the peer closes the connection
    while (true) {
        pthread_mutex_lock(&connectionMutex);
        if (!sendCmd) {
            // printf("\nRecieving\n");
            recvResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
            if (recvResult > 0) {

                sendCmd = true;
                printf("Bytes received: %s\n", recvbuf);

                char *cmd_split = strtok(recvbuf, " "); //divide o comando para pegar só a parte que importa

                // https://stackoverflow.com/questions/3501338/c-read-file-line-by-line
                if (strncmp(cmd_split, "lerdados", DEFAULT_BUFLEN) == 0) {//comando sair do loop
                    // todo: ler valores salvos e enviar até encerrar a leitura

                } else if (strncmp(cmd_split, "getlogtime", DEFAULT_BUFLEN) == 0) {//comando sair do loop
                    char *timedatabuf;
                    itoa(newHz, timedatabuf, 10);
                    sendResult = send(ConnectSocket, timedatabuf, (int) strlen(okbuf), 0);

                    sendCmd = false;
                } else if (strncmp(cmd_split, "setlogtime", DEFAULT_BUFLEN) == 0) {//comando sair do loop

                    pthread_mutex_lock(&savingDataMutex);

                    char *array[3];
                    int i = 0;

                    while (cmd_split != NULL) {
                        array[i++] = cmd_split;
                        cmd_split = strtok(NULL, "/");
                    }
                    newHz = (int) (atoi(array[1]));
                    printf("\nsocket_cmd> New Log Time: %d\n", newHz);
                    sendResult = send(ConnectSocket, okbuf, (int) strlen(okbuf), 0);
                    sendCmd = false;
                    pthread_mutex_unlock(&savingDataMutex);
                }
            } else if (recvResult == 0) {
                sendCmd = true;
                //printf("Connection closed\n");
            } else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                break;
            }
        }
        pthread_mutex_unlock(&connectionMutex);
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


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int) ptr->ai_addrlen);
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

    printf("\nStarting Client\n");

    pthread_t send_thread, recieve_thread, save_thread, can_thread, readAnalog_thread, telemetry_thread, bluetooth_thread;

    pthread_create(&recieve_thread, NULL, socketRecieve, NULL);
    pthread_create(&send_thread, NULL, socketSend, NULL);
    pthread_create(&save_thread, NULL, saveData, NULL);
    pthread_create(&can_thread, NULL, readCanData, NULL);
    pthread_create(&readAnalog_thread, NULL, readAnalogicData, NULL);
    pthread_create(&telemetry_thread, NULL, sendTelemetryData, NULL);
    pthread_create(&bluetooth_thread, NULL, sendBluetoothData, NULL);

    pthread_join(recieve_thread, NULL);
    pthread_join(send_thread, NULL);
    pthread_join(save_thread, NULL);
    pthread_join(can_thread, NULL);
    pthread_join(readAnalog_thread, NULL);
    pthread_join(telemetry_thread, NULL);
    pthread_join(bluetooth_thread, NULL);

    return 0;
}