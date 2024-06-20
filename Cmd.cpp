#include "Cmd.hpp"

void    Server::NICK(Client *client, const std::string &new_nick) {
    bool alr_exists = false;
    for (size_t i = 0; i < _clients.size(); i++) {
        if (new_nick == _clients[i].get_nickname()) {
            alr_exists = true;
            std::string buff = new_nick + " already exists on this server.\n";
            std::cerr << buff;
            send(client->get_fd(), buff.c_str(), buff.size(), 0);
            break;
        }
    }
    if (!alr_exists) {
        client->set_nickname(new_nick);
        std::string confirmation = "Nickname changed to " + new_nick + "\n";
        send(client->get_fd(), confirmation.c_str(), confirmation.size(), 0);
    }
}


void    Server::JOIN(const std::string &chanName, const std::string &nickname, Client *user)
{
    if (_chanMap.find(chanName) != _chanMap.end())
    {
        if (!_chanMap[chanName]->alreadyJoin(this, user, nickname))
        {
            return ;
        }
        _chanMap[chanName]->joinChan(this, user, nickname, chanName);
    }
    else // create a new Channel if chanName not found as a Channel
    {
        Channel *newChan = new Channel(chanName);
        _chanMap.insert(std::make_pair(chanName, newChan));
        _chanMap[chanName]->joinChan(this, user, nickname, chanName);
        _chanMap[chanName]->grantOperator(user, nickname, this, true);
    }
    for (std::map<std::string, Channel*>::iterator it = _chanMap.begin(); it != _chanMap.end(); it++)
    {
        std::cout << it->first << "\r\n";
    }
}




/* ------------------------- Channel ------------------------- */

Channel::Channel(const std::string &name) : _isTopic(false), _chanName(name) { }
Channel::~Channel() { clearMaps(); }


  // -------------------> Join / Create <------------------- //

void    Channel::joinChan(Server *server, Client *user, const std::string &nickname, const std::string &chanName)
{
    _userMap.insert(std::make_pair(nickname, user));
    std::cout << nickname << " join " << chanName << "\r\n";
    RPL(user, server, nickname);
}

bool    Channel::alreadyJoin(Server *server, Client *user, const std::string &nickname)
{
    std::cout << "in function\r\n";
    if (_userMap.find(nickname) != _userMap.end())
    {
        std::cout << "in condition\r\n";
        std::string alrOnChannel = ":" + server->getServerName() + " 338 " + nickname + " " + _chanName + ":You are already on that channel\r\n"; // RPL 338 user already on channel
        send(user->get_fd(), alrOnChannel.c_str(), alrOnChannel.size(), 0);
        return false;
    }
    return true;
}

void    Channel::RPL(Client *user, Server *server, const std::string &nickname)
{
    std::map<std::string, Client*>::iterator it;

    std::string noTopic = ":" + server->getServerName() + " 331 " + nickname + " " + _chanName + " :No topic is set\r\n"; // RPL 331 without topic
    std::string topic = ":" + server->getServerName() + " 332 " + nickname + " " + _chanName + " :" + _topicName + "\r\n"; // RPL 332 with topic

    for (it = _userMap.begin(); it != _userMap.end(); it++)
    {
        std::cout << "user are : " << it->first << "\n";
    } // print server side to get every users on a channel

    std::string namReply = ":" + server->getServerName() + " 353 " + user->get_nickname() + " = " + _chanName + " :"; // RPL 353 list users
    for (it = _userMap.begin(); it != _userMap.end();)
    {
        namReply += it->first + (++it == _userMap.end() ? "" : " ");
    }
    namReply += "\r\n";

    std::string endOfName = ":" + server->getServerName() + " 366 " + user->get_nickname() + " " + _chanName + " :End of /NAME list\r\n"; // RPL 366 end of users list

    if (!_isTopic)
    {
        send(user->get_fd(), noTopic.c_str(), noTopic.size(), 0); // RPL 331
    }
    else
    {
        send(user->get_fd(), topic.c_str(), topic.size(), 0); // RPL 332
    }
    send(user->get_fd(), namReply.c_str(), namReply.size(), 0); // RPL 353
    send(user->get_fd(), endOfName.c_str(), endOfName.size(), 0); // RPL 366
}


  // -------------------> Commands <------------------- //

//void    TOPIC()
void    Server::PART(Client *user, const std::string &chanName, const std::string &reason)
{
    if (_chanMap.find(chanName) != _chanMap.end())
    {
        if (_chanMap[chanName]->isOps(user->get_nickname()))
        {
            _chanMap[chanName]->grantOperator(user, user->get_nickname(), this, false);
        }
        _chanMap[chanName]->eraseUser(user->get_nickname());
        std::string partMsg = ":" + user->get_nickname() + "!" + user->get_nickname() + "@localhost PART " + chanName;
        if (!reason.empty())
            partMsg += " :" + reason;
        partMsg += "\r\n";
        send(user->get_fd(), partMsg.c_str(), partMsg.size(), 0);
    }
    else
    {
        std::string noSuchChannel = ":" + _ServerName + " 403 " + user->get_nickname() + " " + chanName + ":No such channel\r\n";
        send(user->get_fd(), noSuchChannel.c_str(), noSuchChannel.size(), 0);
    }
}

  // -------------------> Others <------------------- //

bool    Channel::isOps(const std::string &nickname)
{
    if (_userOps.find(nickname) != _userOps.end())
        return true;
    return false;
}

void    Channel::eraseUser(const std::string &nickname)
{
    if (_userMap.find(nickname) != _userMap.end())
        _userMap.erase(nickname);
}

// Grant / remove operator access
void    Channel::grantOperator(Client *user, const std::string &nickname, Server *server, bool add)
{
    std::map<std::string, Client*>::iterator it;

    if (add && _userMap.find(nickname) != _userMap.end())
    {
        _userOps.insert(std::make_pair(nickname, user));
    }
    else if (_userOps.find(nickname) != _userOps.end())
    {
        //it = _userOps.find(nickname);
        //delete it->second;
        _userOps.erase(nickname);
    }
    else
    {
        std::string noSuchNick = ":" + server->getServerName() + " 401 " + user->get_nickname() + " " + _chanName + " :No such nick/channel\r\n"; // RPL 401 No such nick
        send(user->get_fd(), noSuchNick.c_str(), noSuchNick.size(), 0);
    }
}

void    Channel::clearMaps()
{
    _userMap.clear();
    _userOps.clear();
}

std::string Channel::getChanName() const { return _chanName; }