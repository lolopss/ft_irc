#pragma once

#include "Utility.hpp"
#include "Client.hpp"
#include "Channel.hpp"

class Channel;

class Server {
private:
    const std::string           _ServerName;
    const std::string           _password;
    int                         _Port;
    int                         _ServerSocketFd;
    static bool                 _Signal;
    std::vector<Client>         _clients;
    std::vector<struct pollfd>  _fds;
    int                         _client_nb;
    std::map<std::string, Channel*> _chanMap;
    std::map<int, std::string>  _partial_messages; // for \D handling

public:
    Server();
    Server(char *port, char *password);
    Server(const Server &o);
    ~Server();
    const Server    operator=(const Server &o);

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
    bool        isHostnameInUse(const std::string &hostname);
    std::string getUniqueHostname(const std::string &hostname);
    bool        isUsernameInUse(const std::string &username);
    std::string getUniqueUsername(const std::string &username);
    
    /*****************Commands(Cmd.cpp)*****************/
    
    void        USER(Client *client, const std::string &username, const std::string &hostname, const std::string &servername, const std::string &realname);
    void        NICK(Client *client, const std::string &new_name);
    void        WHOIS(Client *client, const std::string &target);
    void        JOIN(const std::string &chanName, const std::string &nickname, Client *user, const std::string &password);
    void        LIST(Client *user);
    void        PART(Client *user, const std::string &chanName, const std::string &reason);
    void        TOPIC(Client *client, const std::string &chanName, const std::string &topicName);
    void        PRIVMSG(int sender_fd, const std::string &target, const std::string &message);
    void        PING(int fd, const std::string &message);
    void        INVITE(Client *inviter, const std::string &nickname, const std::string &channelName);
    void        MODE(bool activate, const std::string &chanName, const std::string &mode, Client *client);
    void        PASS(Client *client, const std::string &password);
    void        NAMES(Client *client, const std::string &chanName);

};

