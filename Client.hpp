#pragma once

#include <iostream>
#include <vector> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <poll.h>
#include <csignal> 
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>


#define BUFFER_SIZE 1024
#define RED "\e[1;31m"
#define WHI "\e[0;37m"
#define GRE "\e[1;32m"
#define YEL "\e[1;33m"

class Client {
    private:
        int         _fd;
        std::string _nickname;
        std::string _IPadd; // IP address of the Client 
    public:
        Client() : _fd(-1) {} 
        ~Client() {}

        int         get_fd() const;
        std::string get_nickname() const;
        void        set_nickname(std::string name);
        void        set_fd(int fd);
        void        set_IPADD(const std::string& ip);
};