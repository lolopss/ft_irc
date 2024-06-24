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
        std::string _username;
        std::string _hostname;
        std::string _servername;
        std::string _realname;
        std::string _current_channel;
        bool        _registered;
        std::string _IPadd; // IP address of the Client 
    public:
        Client() : _fd(-1), _nickname(""), _username(""), _hostname(""), _servername(""), _realname(""), _registered(false){} 
        ~Client() {}

        int         get_fd() const;
        void        set_fd(int fd);

        std::string get_nickname() const;
        void        set_nickname(std::string name);

        std::string get_current_channel() { return _current_channel; }
        void        set_current_channel(std::string joined_chan) { _current_channel = joined_chan; }
    
        std::string get_username() const { return _username; }
        void        set_username(const std::string &username) { _username = username; }

        std::string get_hostname() const { return _hostname; }
        void        set_hostname(const std::string &hostname) { _hostname = hostname; }

        std::string get_servername() const { return _servername; }
        void        set_servername(const std::string &servername) { _servername = servername; }

        std::string get_realname() const { return _realname; }
        void        set_realname(const std::string &realname) { _realname = realname; }

        bool        is_registered() const { return _registered; }
        void        set_registered(bool registered) { _registered = registered; }
        void        set_IPADD(const std::string& ip);
};