#pragma once

#include "Server.hpp"

class Server;

class Channel {
    private:
        int                             _nbUsers;
        int                             _userLimit;
        bool                            _topicRestriction;
        bool                            _isTopic;
        std::string                     _topicName;
        std::string                     _chanName;
        bool                            _modeI;
        bool                            _modeT;
        bool                            _modeK;
        bool                            _modeO;
        bool                            _modeL;
        std::string                     _channelPassword;
        std::map<std::string, Client*>  _inviteList;
        // map to get every users datas
        std::map<std::string, Client*>  _userMap;
        // map to get every ops on a channel
        std::map<std::string, Client*>  _userOps;

    public:
        Channel();
        Channel(const std::string &name);
        ~Channel();
        
        void        KICK(Client *operatorClient, const std::string &channelName, const std::string &nickname, const std::string &message);
        std::string getChanName() const;
        int         getNbUser() const;
        bool        alreadyJoin(Server *server, Client *user, const std::string &nickname);
        void    	joinChan(Server *server, Client *user, const std::string &nickname, const std::string &chanName);
        void        chanList(Client *user);
        void        grantOperator(Client *user, const std::string &nickname, Server *server, bool add);
        bool        isOps(const std::string &nickname);
        void        RPL(Client *user, Server *server, const std::string &nickname);
        void        eraseUser(const std::string &nickname);
        void        clearMaps();
        void        addTopic(Client *client, Server *server, const std::string &topicName);
        void        broadcastMessageToChan(const std::string &message, int sender_fd);
        void        addUser(Client *user);
        void        addInviteUser(Client *user);
        bool        isUserInChannel(const std::string &nickname) const;
        bool        isEmpty();
        void        changeNicknameInChannel(Client *user, const std::string &nickname);

        void        handleModeI(bool activate);
        void        handleModeT(bool activate);
        void        handleModeK(bool activate, const std::string &mdp);
        void        handleModeO(bool activate, const std::string &nickname, Client *user, Server *server);
        void        handleModeL(bool activate, const int &userLimit);
        void        setModes(bool activate, const std::string &mode, Client *user, Server *server);
        bool        checkAllModes(Client *user, const std::string &nickname, const std::string &password, Server *server);
};