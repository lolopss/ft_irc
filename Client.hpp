#pragma once

#include <iostream>
#include <vector> //-> for vector
#include <sys/socket.h> //-> for socket()
#include <sys/types.h> //-> for socket()
#include <netinet/in.h> //-> for sockaddr_in
#include <fcntl.h> //-> for fcntl()
#include <unistd.h> //-> for close()
#include <arpa/inet.h> //-> for inet_ntoa()
#include <poll.h> //-> for poll()
#include <csignal> //-> for signal()
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>


#define BUFFER_SIZE 1024
#define RED "\e[1;31m" //-> for red color
#define WHI "\e[0;37m" //-> for white color
#define GRE "\e[1;32m" //-> for green color
#define YEL "\e[1;33m" //-> for yellow color

class Client {
    private:
        int         _fd;
        std::string _nickname;
        std::string _IPadd; // IP address of the Client 

    public:
        Client() : _fd(-1) {} // Initialize _fd to -1
        ~Client() {}

        int         get_fd() const;
        std::string get_nickname() const;
        void        set_nickname(std::string name);
        void        set_fd(int fd);
        void        set_IPADD(const std::string& ip);
};