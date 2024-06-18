#pragma once

#include "Server.hpp"

class Server;

class Channel {
private:
    bool        _isTopic;
    std::string _topicName;
    std::string _chanName;
    // container map pour avoir les donnees de chaque utilisateurs
    std::map<std::string, Client*> _userMap;
public:
    Channel();
    Channel(const std::string &name);
    ~Channel();

    std::string getChanName() const;
    bool        alreadyExist(Server *server, Client *user, const std::string nickname);
    void    	joinChan(Server *server, Client *user, const std::string &nickname, const std::string &chanName);
    void        RPL(Client *user, Server *server, const std::string &nickname);
};

    //JOIN()
    //create()
    //join()
    //cmd()