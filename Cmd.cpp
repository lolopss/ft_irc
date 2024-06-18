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


// gerer les cmds et modes

void    Server::JOIN(const std::string &chanName, const std::string &nickname, Client *user)
{
    if (_chanMap.find(chanName) != _chanMap.end())
    {
        if (!_chanMap[chanName]->alreadyExist(this, user, nickname))
            return ;
        _chanMap[chanName]->joinChan(this, user, nickname, chanName);
    }
    else
    {
        Channel *newChan = new Channel(chanName);
        _chanMap.insert(std::make_pair(chanName, newChan));
        _chanMap[chanName]->joinChan(this, user, nickname, chanName);
    }
    for (std::map<std::string, Channel*>::iterator it = _chanMap.begin(); it != _chanMap.end(); it++)
    {
        std::cout << it->first << "\r\n";
    }
}




/* ------------------------- Channel ------------------------- */

Channel::Channel(const std::string &name) : _isTopic(false), _chanName(name) { }

bool    Channel::alreadyExist(Server *server, Client *user, const std::string nickname)
{
    if (_userMap.find(nickname) != _userMap.end())
    {
        std::string endOfName = ":" + server->getServerName() + " 338 " + nickname + " #" + _chanName + ":You are already on that channel\r\n"; // RPL 338 utilisateur deja sur le channel
        return false;
    }
    return true;
}

void    Channel::joinChan(Server *server, Client *user, const std::string &nickname, const std::string &chanName)
{
    _userMap.insert(std::make_pair(nickname, user));
    std::cout << nickname << " join " << chanName << "\r\n";
    RPL(user, server, nickname);
}

void    Channel::RPL(Client *user, Server *server, const std::string nickname)
{
    std::string noTopic = ":" + server->getServerName() + " 331 " + nickname + " #" + _chanName + " :No topic is set\r\n"; // RPL 331 pas de topic
    std::string topic = ":" + server->getServerName() + " 332 " + nickname + " #" + _chanName + " :" + _topicName + "\r\n"; // RPL 332 avec topic

    for (std::map<std::string, Client*>::iterator it = _userMap.begin(); it != _userMap.end(); it++)
    {
        std::cout << "user are : " << it->first << "\n";
    } // print cote serveur pour le debugage

    std::string namReply = ":" + server->getServerName() + " 353 " + user->get_nickname() + " = #" + _chanName + " :"; // RPL 353 liste des utilisateurs
    for (std::map<std::string, Client*>::iterator it = _userMap.begin(); it != _userMap.end();)
    {
        namReply += it->first + (++it == _userMap.end() ? "\r\n" : " ");
    }
    std::string endOfName = ":" + server->getServerName() + " 366 " + user->get_nickname() + " #" + _chanName + ":End of /NAME list\r\n"; // RPL 366 fin de liste des utilisateurs

    if (!_isTopic){
        send(user->get_fd(), noTopic.c_str(), noTopic.size(), 0); // RPL 331
    }
    else{
        send(user->get_fd(), topic.c_str(), topic.size(), 0); // RPL 332
    }
    send(user->get_fd(), namReply.c_str(), namReply.size(), 0); // RPL 353
    send(user->get_fd(), endOfName.c_str(), endOfName.size(), 0); // RPL 366
}

std::string Channel::getChanName() const { return _chanName; }