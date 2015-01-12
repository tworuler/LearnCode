#include <cstdio>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <map>
#include <fcntl.h>
#include <mysql/mysql.h>
using namespace std;

#define DEBUG

const char LOGIN_SUCCESS[] = "LOGIN_SUCCESS";
const char CHECK_LOGIN[] = "CHECK_LOGIN";
const char EXIT_CMD[] = "EXIT";
const char TO_ALL[] = "TO_ALL"; 
const int MAXCLIENTS = 1024;

struct epoll_event events[MAXCLIENTS];
map<string, int> users;
int serverSocketFd;
int epollFd;

MYSQL *mysql;
MYSQL_RES *res;
MYSQL_ROW row;

void debug(string message) {
#ifdef DEBUG
    cout << message << endl;
#endif
}

void log(string message) {
    cout << message << endl;
}

int createServer(int port) {
    int serverSockedFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSockedFd == -1) {
        log("Create server socked fail");
        exit(-1);
    }

    int flag = 1;
    setsockopt(serverSockedFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&flag, sizeof(flag));

    struct sockaddr_in stBindAddr;
    stBindAddr.sin_family = AF_INET;
    stBindAddr.sin_port = htons(port);
    stBindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSockedFd, (struct sockaddr*)&stBindAddr, sizeof(stBindAddr)) == -1) {
        log("Bind address error");
        exit(-1);
    }

    if (listen(serverSockedFd, MAXCLIENTS) == -1) {
        log("Listen error");
        exit(-1);
    }

    return serverSockedFd;
}

int setNonBlocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        log("set non block error");
        return -1;
    }
    flag |= O_NONBLOCK;
    int s = fcntl(fd, F_SETFL, flag);
    if (s == -1) {
        log("set non block error");
        return -1;
    }
}

void sendToAll(string username, string message) {
    for (auto& it : users) {
        if (it.first != username)
            write(it.second, message.c_str(), message.size());
    }
}

void sendToOne(string username, string to_user, string message) {
    if (users.find(to_user) != users.end()) {
        write(users[to_user], message.c_str(), message.size());
    }
}

void work_data(char *str, int fd) {
    char *p = strchr(str, '#');
    *p = 0;
    if (strcmp(str, CHECK_LOGIN) == 0) {
        str = p + 1;
        p = strchr(str, '#');
        *p = 0;
        string username = str;
        string password = p + 1;
        
        //TODO check login
        string query = "select * from user where username='" + username + "'";
        debug(query);
        if (mysql_query(mysql, query.c_str()) != 0) {        
            res = mysql_use_result(mysql);
            //row = mysql_fetch_row(res);
            if (true || res) {
                users[str] = fd; 
                write(fd, LOGIN_SUCCESS, sizeof(LOGIN_SUCCESS));
                sendToAll(username, "[System]: " + username + " on line.");
            }
        } else {
            close(fd);
            write(fd, "LOGIN_FAIL", sizeof("LOGIN_FAIL"));
        }
    } else {
        string username = str;
        str = p + 1;
        p = strchr(str, '#');
        if (p != NULL)
            *p = 0;
        if (strcmp(str, EXIT_CMD) == 0) {
            close(users[username]);
            users.erase(username);
            sendToAll(username, "[System]: " + username + " off line.");
        } else if (strcmp(str, TO_ALL) == 0) {
            str = p + 1;
            sendToAll(username, "[" + username + "]: " + str);  
        } else {
            string to_user = str;
            str = p + 1;
            sendToOne(username, to_user, "[" + username + " to you]: " + str);
        }
    }
}

void epoll_loop() {
    struct epoll_event event;
    event.data.fd = serverSocketFd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocketFd, &event)) {
        log("Epoll ctl error");
        exit(-1);
    }

    while (1) {
        int count = epoll_wait(epollFd, events, MAXCLIENTS, -1);
        for (int i = 0; i < count; ++i) {
            if (events[i].data.fd == serverSocketFd) {
                for (;;) {
                    event.data.fd = accept(serverSocketFd, NULL, NULL);
                    if (event.data.fd > 0) {
                        setNonBlocking(event.data.fd);
                        event.events = EPOLLIN | EPOLLET;
                        epoll_ctl(epollFd, EPOLL_CTL_ADD, event.data.fd, &event);
                    } else {
                        if (errno == EAGAIN)
                            break;
                    }
                }
            } else {
                if (events[i].events & EPOLLIN) {
                    char buf[1024];
                    ssize_t iRet = read(events[i].data.fd, buf, sizeof(buf) - 1);
                    if (iRet == -1) {
                        log("read error");
                    } else if (iRet == 0) {
                        log("Client request close");
                        // TODO close fd etc.
                    } else {
                        buf[iRet] = 0;
                        debug(buf);
                        work_data(buf, events[i].data.fd); 
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int port = 8888;

    if (argc > 1) {
        if (sscanf(argv[2], "%d", &port) != -1) {
            log("Illegal port");
            exit(-1);
        }
    }

    serverSocketFd = createServer(port);
    if (setNonBlocking(serverSocketFd) == -1)
        return -1;

    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        log("Epoll create error");
        return -1;
    }
    
    mysql = mysql_init(NULL);
    if (mysql == NULL) {
        log("mysql fail");
        return -1;
    }
    char password[10];
    FILE *fp = fopen("/root/temp/password", "r");
    fscanf(fp, "%s", password);
    if (mysql_real_connect(mysql, "localhost", "root", password, NULL, 0, NULL, 0) == NULL) {
        mysql_query(mysql, "use chatroom");
        log("mysql real connect fail");
        return -1;
    }

    epoll_loop();
    
    return 0;
}

