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
#include <algorithm>


#define BUFFER_SIZE 1024
#define RED "\e[1;31m"
#define WHI "\e[0;37m"
#define GRE "\e[1;32m"
#define YEL "\e[1;33m"

class Client {
    private:
        int                         _fd;
        std::string                 _nickname;
        std::string                 _username;
        std::string                 _hostname;
        std::string                 _servername;
        std::string                 _realname;
        std::string                 _current_channel;
        std::vector<std::string>    _channelList;
        bool                        _registered;
        std::string                 _IPadd; // IP address of the Client 
        bool                        _invisible;
        bool                        _authenticated; // Track if the client is authenticated
    public:
        Client() : _fd(-1), _nickname(""), _username(""), _hostname(""), _servername(""), _realname(""), _registered(false), _invisible(false), _authenticated(false){} 
        ~Client() {}

        int         get_fd() const;
        void        set_fd(int fd);

        std::string getID(void);

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

        std::vector<std::string>    get_channelList();

        bool        is_registered() const { return _registered; }
        void        set_registered(bool registered) { _registered = registered; }
        
        void        authenticate() { _authenticated = true; }
        bool        is_authenticated() const { return _authenticated; }

        void        set_IPADD(const std::string& ip);
        std::string get_IPADD() const { return _IPadd; }
        void        handlePartCommand(const std::string &channelName);

        void        insertChannel(const std::string &chanName);
        void        removeChannel(const std::string &chanName);
};