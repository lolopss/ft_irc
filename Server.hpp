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
#include "Channel.hpp"

class Channel;

class Server {
private:
    const std::string           _ServerName;
    int                         _Port;
    int                         _ServerSocketFd;
    static bool                 _Signal;
    std::vector<Client>         _clients;
    std::vector<struct pollfd>  _fds;
    int                         _client_nb;
    std::map<std::string, Channel*> _chanMap;
    std::map<int, std::string> _partial_messages; // for \D handling

public:
    Server(char *port) : _ServerName("PEERC"), _Port(atoi(port)), _ServerSocketFd(-1) {}
    ~Server() {}

    std::string getClientNickname(int client_fd);
    std::string getServerName() const;
    Channel     *get_Channel(const std::string &chanName);
    Client      *findClientByNickname(const std::string& nickname);
    Client      *getClientByFd(int fd);
    void        sendWelcomeMessages(int client_fd);
    void        serverInit();
    void        serverSocket();
    void        acceptNewClient();
    void        receiveNewData(int fd);
    static void SignalHandler(int signum);
    void        closeFds();
    void        clearMap();
    
    void        clearClients(int fd);
    void        broadcastMessage(const std::string &message, int sender_fd);
    bool        exec_command(std::istringstream &iss, const std::string &command, Client &client, const std::string &msg);
    void        run();
    
    
    /*****************Commands(Cmd.cpp)*****************/
    
    void        USER(Client *client, const std::string &username, const std::string &hostname, const std::string &servername, const std::string &realname);
    void        NICK(Client *client, const std::string &new_name);
    void        JOIN(const std::string &chanName, const std::string &nickname, Client *user);
    void        LIST(Client *user);
    void        PART(Client *user, const std::string &chanName, const std::string &reason);
    void        TOPIC(Client *client, const std::string &chanName, const std::string &topicName);
    void        PRIVMSG(int sender_fd, const std::string &target, const std::string &message);
    void        PING(int fd, const std::string &message);
    void        INVITE(Client *inviter, const std::string &nickname, const std::string &channelName);
    void        MODE(bool activate, const std::string &chanName, const std::string &mode, Client *client);
};

