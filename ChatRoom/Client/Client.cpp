#include <cstdio>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
using namespace std;

char username[100];
int socketFd;

void log(const char *message) {
    cout << message << endl;
}

int connect(const char *ip, int port) {
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
        log("Create socket failed.");
        exit(-1);
    }

    struct sockaddr_in stSvrAddr;
    stSvrAddr.sin_family = AF_INET;
    stSvrAddr.sin_port = htons(port);
    stSvrAddr.sin_addr.s_addr = inet_addr(ip);
    if (connect(socketFd, (struct sockaddr*)&stSvrAddr, sizeof(stSvrAddr)) == -1) {
        log("Connect to server error");
        exit(-1);
    }

    return socketFd;
}

void login(int socketFd) {
    cout << "Enter your username and password:" << endl;
    char password[100];
    cin >> username >> password;
    string message = string(username) + ' ' + password;
    write(socketFd, message.c_str(), message.size());
    char buf[1024];
    read(socketFd, buf, sizeof(buf) - 1);
    if (strcmp(buf, "OK") != 0) {
        cout << "Login fail" << endl;
        exit(0);
    } else {
        cout << "Login successful" << endl;
    }
}

void* recieve(void *arg) {

}

void* send(void *arg) {

}

int main(int argc, char** argv) {
    char ip[100] = "127.0.0.1";
    int port = 8888;

    if (argc > 1) {
        strcpy(ip, argv[1]);
        // TODO check ip
    }
    if (argc > 2) {
        if (sscanf(argv[2], "%d", &port) != 1) {
            log("Illegal port");
            exit(-1);
        }
    }

    socketFd = connect(ip, port);
    login(socketFd);
    
    pthread_t recvThread;
    pthread_t sendThread;
    if (pthread_create(&recvThread, NULL, recieve, NULL) != 0) {
        log("Create recieve thread fail");
        exit(-1);
    }
    if (pthread_create(&sendThread, NULL, send, NULL) != 0) {
        log("Create send thread fail");
        exit(-1);
    }

    while (1);

    return 0;
}

