#pragma once

#include <vector>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sstream>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <map>
#include <utility> // std::pair / std::make_pair
#include <iterator>
#include "Client.hpp"
#include "Cmd.hpp"

class Channel;

class Server {
private:
    std::string                 _ServerName;
    int                         _Port;
    int                         _ServerSocketFd;
    static bool                 _Signal;
    std::vector<Client>         _clients;
    std::vector<struct pollfd>  _fds;
    int                         _client_nb;
    // container map pour les Channels
    std::map<std::string, Channel*> _chanMap;

public:
    Server() : _Port(4444), _ServerSocketFd(-1) {}
    Server(char *port) : _Port(atoi(port)), _ServerSocketFd(-1) {}
    ~Server() {}

    std::string getClientNickname(int client_fd);
    std::string getServerName() const;
    void        sendWelcomeMessages(int client_fd);
    void        serverInit();
    void        serverSocket();
    void        acceptNewClient();
    void        receiveNewData(int fd);
    static void SignalHandler(int signum);
    void        closeFds();
    void        clearClients(int fd);
    void        broadcastMessage(const std::string &message, int sender_fd);
    int         exec_command(std::istringstream &iss, std::string &command, std::vector<Client> &_clients, int &fd);
    void        run();
    
    
    /*****************Commands(Cmd.cpp)*****************/
    
    void        NICK(Client *client, const std::string &new_name);
    void        JOIN(const std::string &chanName, const std::string &nickname, Client *user);
    void        PRIVMSG(int sender_fd, const std::string &target, const std::string &message);
};

