#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <cstdio>
using namespace std;

#define MAXEVENTS 1024

int createServer()
{
    int iSockFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == iSockFd)
    {
        cout << "socket error|msg:" << ::strerror(errno) << std::endl;
        return (-1);
    }

    int iFlag = 1;
    ::setsockopt(iSockFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&iFlag, sizeof(iFlag));

    struct sockaddr_in stBindAddr;
    stBindAddr.sin_family = AF_INET;
    stBindAddr.sin_port = htons(8888);
    stBindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(::bind(iSockFd, (struct sockaddr*)&stBindAddr, sizeof(stBindAddr)) == -1)
    {
        cout << "bind address error|msg:" << ::strerror(errno) << std::endl;

        return (-1);
    }

    if(::listen(iSockFd, 1024) == -1)
    {
        cout << "listen error|msg:" << ::strerror(errno) << std::endl;
        return (-1);
    }
    
    return (iSockFd);
}

int setNonBlocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        puts("set non block error");
        return -1;
    }
    flag |= O_NONBLOCK;
    int s = fcntl(fd, F_SETFL, flag);
    if (s == -1) {
        puts("set non block error");
        return -1;
    }
}

struct epoll_event events[MAXEVENTS];
int main(int argc, char *argv[]) {
    int serverSocket_fd = createServer();
    if (serverSocket_fd == -1)
        return -1;

    if(setNonBlocking(serverSocket_fd) == -1)
        return -1;
    
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        puts("epoll create error");
        return -1;
    }

    struct epoll_event event;

    event.data.fd = serverSocket_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket_fd, &event)) {
        puts("epoll ctl error");
        return -1;
    }

    for (;;) {
        int n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == serverSocket_fd) {
                for (;;) {
                    event.data.fd = accept(serverSocket_fd, NULL, NULL);
                    if (event.data.fd > 0) {
                        setNonBlocking(event.data.fd);
                        event.events = EPOLLIN | EPOLLET;
                        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event);
                    } else {
                        if (errno == EAGAIN)
                            break;
                    }
                }
            } else {
                if (events[i].events & EPOLLIN) {
                    char buf[1024];
                    ssize_t iRet = read(events[i].data.fd, buf, sizeof(buf) - 1);
                    if (-1 == iRet) {
                        cout << "read error" << endl;
                    } else if (0 == iRet) {
                        cout << "client request to close connection" << endl;
                    } else {
                        buf[iRet] = '\0';
                        write(events[i].data.fd, buf, (size_t)iRet);
                        cout << "recv client message|msg_len:" << iRet << "|msg_content:"
                                              << buf << std::endl;
                    }
                    epoll_ctl(events[i].data.fd, EPOLL_CTL_DEL, events[i].data.fd, &event);
                    close(events[i].data.fd);
                }
            }
        }
    }
}
