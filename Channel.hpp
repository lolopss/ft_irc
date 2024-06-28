#pragma once

#include "Server.hpp"

class Server;

class Channel {
    private:
        int                 _nbUsers;
        const int           _userLimit;
        bool                _topicRestriction;
        bool                _isTopic;
        std::string         _topicName;
        std::string         _chanName;
        const bool          _modeI;
        const bool          _modeT;
        const bool          _modeK;
        const bool          _modeO;
        const bool          _modeL;
        const std::string   _channelPassword;
        // map to get every users datas
        std::map<std::string, Client*> _userMap;
        // map to get every ops on a channel
        std::map<std::string, Client*> _userOps;

    public:
        Channel();
        Channel(const std::string &name);
        ~Channel();

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
        bool        isUserInChannel(const std::string &nickname) const;
        bool        isEmpty();

        void        handleModeI(bool activate) const;
        void        handleModeT(bool activate) const;
        void        handleModeK(bool activate, const std::string &mdp) const;
        void        handleModeO(bool activate, const std::string &nickname);
        void        handleModeL(bool activate, const int &userLimit) const;
};