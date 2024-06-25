#include "Cmd.hpp"

void Server::USER(Client *client, const std::string &username, const std::string &hostname, const std::string &servername, const std::string &realname) {
    if (client->is_registered()) {
        std::string response = ":server 462 * :You may not reregister\r\n";
        send(client->get_fd(), response.c_str(), response.size(), 0);
        return;
    }

    client->set_username(username);
    client->set_hostname(hostname);
    client->set_servername(servername);
    client->set_realname(realname);

    client->set_registered(true);
}

void Server::NICK(Client *client, const std::string &new_nick) {
    // Vérification si le surnom est fourni
    if (new_nick.empty()) {
        std::string response = ":server 431 * :No nickname given\r\n";
        send(client->get_fd(), response.c_str(), response.size(), 0);
        return;
    }

    // Vérification si le surnom est déjà utilisé
    bool alr_exists = false;
    for (size_t i = 0; i < _clients.size(); i++) {
        if (new_nick == _clients[i].get_nickname()) {
            alr_exists = true;
            std::string response = ":server 433 * " + new_nick + " :Nickname is already in use\r\n";
            send(client->get_fd(), response.c_str(), response.size(), 0);
            return ;
        }
    }
    std::string old_nick = client->get_nickname();
    client->set_nickname(new_nick);
    std::string confirmation = ":server 001 " + new_nick + " :Nickname changed to " + new_nick + "\r\n";
    std::cout << "Client " << client->get_fd() << " changed nickname from " << old_nick << " to " << new_nick << "\n";
    send(client->get_fd(), confirmation.c_str(), confirmation.size(), 0);
}

/*void    Server::JOIN(const std::string &chanName, const std::string &nickname, Client *user)
{
    if (_chanMap.find(chanName) != _chanMap.end())
    {
        // if (!_chanMap[chanName]->alreadyJoin(this, user, nickname))
        // {
        //     return ;
        // }
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
}*/ //ANCIENNE JOIN (je laisse au cas ou)

Channel *Server::get_Channel(const std::string &chanName){
    std::map<std::string, Channel*>::iterator it = _chanMap.find(chanName);
    if (it != _chanMap.end()) {
        return it->second;
    }
    return NULL;
}

void Channel::broadcastMessageToChan(const std::string &message, int sender_fd) {
    for (std::map<std::string, Client*>::iterator it = _userMap.begin(); it != _userMap.end(); ++it) {
        if (it->second->get_fd() != sender_fd) {
            send(it->second->get_fd(), message.c_str(), message.size(), 0);
        }
    }
}

void Server::JOIN(const std::string &chanName, const std::string &nickname, Client *user) {
    std::map<std::string, Channel*>::iterator it = _chanMap.find(chanName);
    if (it == _chanMap.end()) {
        Channel *newChannel = new Channel(chanName);
        _chanMap[chanName] = newChannel;
    }
    _chanMap[chanName]->addUser(user);
    std::string joinMsg = ":" + user->get_nickname() + " JOIN :" + chanName + "\r\n";
    send(user->get_fd(), joinMsg.c_str(), joinMsg.size(), 0);
    // Send RPL messages to the joining user
    _chanMap[chanName]->RPL(user, this, nickname);
    // Broadcast join message to other users in the channel
    _chanMap[chanName]->broadcastMessageToChan(joinMsg, user->get_fd());
    user->set_current_channel(chanName);
}

void    Server::LIST(Client *user)
{
    std::map<std::string, Channel*>::iterator it;
    std::string header = "321 Channels :Users Name\r\n"; // RPL 321 list start
    std::string listEnd = ":End of /LIST\r\n";

    send(user->get_fd(), header.c_str(), header.size(), 0);
    for (it = _chanMap.begin(); it != _chanMap.end(); it++)
    {
        it->second->chanList(user);
    }

    send(user->get_fd(), listEnd.c_str(), listEnd.size(), 0);
}



/* ------------------------- Channel ------------------------- */

Channel::Channel(const std::string &name) : _nbUsers(0), _topicRestriction(false), _isTopic(false), _chanName(name) { }
Channel::~Channel() { clearMaps(); }


  // -------------------> Join / Create <------------------- //

void    Channel::joinChan(Server *server, Client *user, const std::string &nickname, const std::string &chanName)
{
    _nbUsers++;
    _userMap.insert(std::make_pair(nickname, user));
    std::cout << nickname << " joined channel #" << chanName << "\r\n";
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

void    Channel::chanList(Client *user)
{
    std::ostringstream  oss;
    oss << _nbUsers;

    std::string list = "322 " + _chanName + " " + oss.str() + ":" + _topicName + "\r\n"; // RPL 322 list
    send(user->get_fd(), list.c_str(), list.size(), 0);
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
        send(user->get_fd(), noTopic.c_str(), noTopic.size(), 0); // RPL 331
    else
        send(user->get_fd(), topic.c_str(), topic.size(), 0); // RPL 332
    send(user->get_fd(), namReply.c_str(), namReply.size(), 0); // RPL 353
    send(user->get_fd(), endOfName.c_str(), endOfName.size(), 0); // RPL 366
}


  // -------------------> Commands <------------------- //

void    Server::TOPIC(Client *user, const std::string &chanName, const std::string &topicName)
{
    if (_chanMap.find(chanName) != _chanMap.end())
    {
        _chanMap[chanName]->addTopic(user, this, topicName);
    }
    else
    {
        std::string error_message = ":" + _ServerName + " 442 " + user->get_nickname() + " " + chanName + " :You're not on that channel\r\n";
        send(user->get_fd(), error_message.c_str(), error_message.size(), 0);
    }
}

void    Channel::addTopic(Client *user, Server *server, const std::string &topicName)
{
    if (!topicName.empty())
    {
        if (topicName == ":")
        {
            _topicName = "";
            std::string setTopic = ":" + user->get_nickname() + "!" + user->get_nickname() + "@localhost " + _chanName + " :" + _topicName + "\r\n";
            send(user->get_fd(), setTopic.c_str(), setTopic.size(), 0);
        }
        else
        {
            _topicName = topicName.substr(1);
            std::string setTopic = ":" + user->get_nickname() + "!" + user->get_nickname() + "@localhost " + _chanName + " :" + _topicName + "\r\n";
            send(user->get_fd(), setTopic.c_str(), setTopic.size(), 0);
        }
    }
    else
    {
        std::string topic = ":" + server->getServerName() + " 332 " + user->get_nickname() + " " + _chanName + " :" + _topicName + "\r\n";
        send(user->get_fd(), topic.c_str(), topic.size(), 0);
    }
}

void Client::handlePartCommand(const std::string &channelName) {
    if (_current_channel == channelName)
        _current_channel.clear();
}

void Server::PART(Client *user, const std::string &chanName, const std::string &reason) {
    std::map<std::string, Channel*>::iterator it = _chanMap.find(chanName);
    if (it != _chanMap.end()) {
        Channel *channel = it->second;
        if (channel->isUserInChannel(user->get_nickname())) {
            std::string partMsg = ":" + user->get_nickname() + "!" + user->get_username() + "@" + user->get_hostname() + " PART " + chanName;
            if (!reason.empty()) {
                partMsg += " :" + reason;
            }
            partMsg += "\r\n";
            send(user->get_fd(), partMsg.c_str(), partMsg.size(), 0);
            channel->broadcastMessageToChan(partMsg, user->get_fd());
            user->handlePartCommand(chanName);
            channel->eraseUser(user->get_nickname());
            if (channel->getNbUser() == 0) {
                std::cout << "in condition\r\n";
                std::map<std::string, Channel*>::iterator test;
                for (test = _chanMap.begin(); test != _chanMap.end(); test++)
                {
                    std::cout << "before" << test->first << "\r\n";
                }
                delete it->second;
                _chanMap.erase(it);
                //delete channel;
                for (test = _chanMap.begin(); test != _chanMap.end(); test++)
                {
                    std::cout << "after" << test->first << "\r\n";
                }
                std::cout << "afterafter\r\n";
            }
        } else {
            std::string error_message = ":server 442 " + user->get_nickname() + " " + chanName + " :You're not on that channel\r\n";
            send(user->get_fd(), error_message.c_str(), error_message.size(), 0);
        }
    } else {
        std::string error_message = ":server 403 " + user->get_nickname() + " " + chanName + " :No such channel\r\n";
        send(user->get_fd(), error_message.c_str(), error_message.size(), 0);
    }
    std::cout << user->get_current_channel() << "= CURRENT CHANNEL JSGDFBKJGBFDA\n";
}

bool Channel::isEmpty()
{
	if (_userMap.empty())
		return true;
	return false;
}

std::string Client::getID()
{
	std::string ID = ":" + this->_nickname + "!" + this->_username + "@" + this->_IPadd;
	return (ID);
}

// void	Server::PART(Client *user, const std::string &reason)
// {
// 	std::map<std::string, Channel*>::iterator it;
// 	for (it = _chanMap.begin(); it != _chanMap.end(); ++it)
// 	{
// 		std::string name = it->first;
// 		Channel* channel = it->second;
//             std::cout << "NAME = : " << name << " AND REASON = " << reason << std::endl;
        
// 		if (reason == name)
// 		{
//             std::cout << "mgfdalkngsfdgsdgsdfgsdfgsdf\n";
// 			std::string response4 = user->getID() + " PART " + channel->getChanName() + " :" + user->get_nickname() + "\r\n";
// 			send(user->get_fd(), response4.c_str(), response4.size(), 0);
// 			channel->broadcastMessageToChan(response4, user->get_fd());
// 			user->set_current_channel("");
// 			channel->eraseUser(user->get_nickname());
// 			if (channel->isEmpty() == true)
// 			{
// 				delete channel;
// 				_chanMap.erase(it);
// 			}
// 			return;
// 		}
// 	}
// }

void Server::PRIVMSG(int sender_fd, const std::string &target, const std::string &message) {
    std::string sender_nick = getClientNickname(sender_fd);
    std::string full_message = ":" + sender_nick + " PRIVMSG " + target + " :" + message + "\r\n";
    
    // Check if the target is a user
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_nickname() == target) {
            int target_fd = _clients[i].get_fd();
            send(target_fd, full_message.c_str(), full_message.size(), 0);
            std::cout << "Sent private message from " << sender_nick << " to " << target << ": " << message << std::endl;
            return;
        }
    }

    // Check if the target is a channel
    std::map<std::string, Channel*>::iterator chanIt = _chanMap.find(target);
    if (chanIt != _chanMap.end()) {
        chanIt->second->broadcastMessageToChan(full_message, sender_fd);
        std::cout << "Sent channel message from " << sender_nick << " to channel " << target << ": " << message << std::endl;
        return;
    }
    
    // If target not found, notify sender
    std::string error_message = ":" + _ServerName + " 401 " + sender_nick + " " + target + " :No such nick/channel\r\n";
    send(sender_fd, error_message.c_str(), error_message.size(), 0);
}

  // -------------------> Others <------------------- //

bool    Channel::isOps(const std::string &nickname)
{
    if (_userOps.find(nickname) != _userOps.end())
        return true;
    return false;
}

bool Channel::isUserInChannel(const std::string &nickname) const {
    return _userMap.find(nickname) != _userMap.end();
}

void Channel::addUser(Client *user) {
    _userMap[user->get_nickname()] = user;
    _nbUsers++;
    std::cout << "User " << user->get_nickname() << " added to channel " << _chanName << ". Current users: " << _nbUsers << std::endl;
}

void    Channel::eraseUser(const std::string &nickname)
{
    if (_userMap.find(nickname) != _userMap.end()){
        _userMap.erase(nickname);
        _nbUsers--;
        std::cout << "User " << nickname << " removed from channel " << _chanName << ". Current users: " << _nbUsers << std::endl;
    }
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

int         Channel::getNbUser() const { return _nbUsers; }
void Server::PING(int fd, const std::string &msg) {
    std::string pong_response = "PONG " + msg + "\r\n";
    send(fd, pong_response.c_str(), pong_response.size(), 0);
    std::cout << "Sent to " << fd << ": " << pong_response << std::endl;
}
