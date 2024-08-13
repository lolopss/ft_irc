#include "Channel.hpp"


void    Server::USER(Client *client, const std::string &username, const std::string &hostname, const std::string &servername, const std::string &realname) {
    if (client->is_registered()) {
        std::string response = ":server 462 * :You may not reregister\r\n";
        send(client->get_fd(), response.c_str(), response.size(), MSG_NOSIGNAL);
        return;
    }

    std::string uniqueUsername = getUniqueUsername(username);
    client->set_username(uniqueUsername);
    std::string uniqueHostname = getUniqueHostname(hostname);
    client->set_hostname(uniqueHostname);
    client->set_servername(servername);
    client->set_realname(realname);
    client->set_registered(true);

    //get ip_ADD
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client->get_fd(), (struct sockaddr *)&client_addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    client->set_IPADD(client_ip);
}


void    Server::WHOIS(Client *client, const std::string &target) // to test WHOIS command
{
    Client  *user = NULL;
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_nickname() == target) {
            user = &_clients[i];
            break;
        }
    }
    if (user == NULL)
        return ;

    std::string targetChannelList;
    for (unsigned i = 0; i < user->get_channelList().size(); i++)
    {
        targetChannelList += user->get_channelList()[i] + (i + 1 < user->get_channelList().size() ? " " : "\r\n");
    }
    std::string whoIsUser = ":" + _ServerName + " 311 " + client->get_nickname() + " " + user->get_nickname() + " " + user->get_username() + " " + user->get_hostname() + " * :" + user->get_realname() + "\r\n";
    std::string whoIsServer = ":" + _ServerName + " 312 " + client->get_nickname() + " " + user->get_nickname() + " " + _ServerName + " :Homemade IRC serv\r\n";
    std::string endOfWhois = ":" + _ServerName + " 318 " + client->get_nickname() + " " + user->get_nickname() + " :End of WHOIS list\r\n";
    std::string whoIsChannels = ":" + _ServerName + " 319 " + client->get_nickname() + " " + user->get_nickname() + " :" + targetChannelList + "\r\n";

    send(client->get_fd(), whoIsUser.c_str(), whoIsUser.size(), MSG_NOSIGNAL);
    send(client->get_fd(), whoIsServer.c_str(), whoIsServer.size(), MSG_NOSIGNAL);
    send(client->get_fd(), whoIsChannels.c_str(), whoIsChannels.size(), MSG_NOSIGNAL);
    send(client->get_fd(), endOfWhois.c_str(), endOfWhois.size(), MSG_NOSIGNAL);
}

static std::string  strToupper(std::string str)
{
    std::string ret;
    for (unsigned i = 0; i < str.length(); i++)
    {
        ret += std::toupper(str[i]);
    }

    std::cout << ret << std::endl;
    return ret;
}

void    Server::NICK(Client *client, const std::string &new_nick) {
    if (new_nick.empty()) {
        std::string response = ":" + _ServerName + " 431 * :No nickname given\r\n";
        send(client->get_fd(), response.c_str(), response.size(), MSG_NOSIGNAL);
        return;
    }
    if (new_nick.find('#'))
    {
        std::string response = client->get_username() + " " + client->get_nickname() + " :Erroneus nickname\r\n";

    }
    for (size_t i = 0; i < _clients.size(); i++) {
        if (strToupper(new_nick) == strToupper(_clients[i].get_nickname())) {
            std::string response = ":" + _ServerName + " 433 * " + new_nick + " :Nickname is already in use\r\n";
            send(client->get_fd(), response.c_str(), response.size(), MSG_NOSIGNAL);
            return ;
        }
    }
    std::string old_nick = client->get_nickname();
    client->set_nickname(new_nick);
    // function change nickname in channel
    for (unsigned i = 0; i < client->get_channelList().size(); i++)
    {
        _chanMap[client->get_channelList()[i]]->changeNicknameInChannel(client, old_nick);
    }
    std::string confirmation = ":" + _ServerName + " 001 " + new_nick + " :Nickname changed to " + new_nick + "\r\n";
    std::cout << GRE << "Client " << client->get_fd() << BLU << " changed nickname from " << old_nick << " to " << new_nick << WHI<< "\n";
    send(client->get_fd(), confirmation.c_str(), confirmation.size(), MSG_NOSIGNAL);
}

void Server::INVITE(Client *inviter, const std::string &nickname, const std::string &channelName) {
    // Find the invited user
    Client *invitedUser = findClientByNickname(nickname);
    if (!invitedUser) {
        std::string error_message = ":" + _ServerName + " 401 " + inviter->get_nickname() + " " + nickname + " :No such nick/channel\r\n";
        send(inviter->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        return;
    }

    // Find the channel
    std::map<std::string, Channel*>::iterator it = _chanMap.find(channelName);
    if (it == _chanMap.end()) {
        std::string error_message = ":" + _ServerName + " 403 " + inviter->get_nickname() + " " + channelName + " :No such channel\r\n";
        send(inviter->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        return;
    }
    Channel *channel = it->second;

    // Check if inviter is in the channel
    if (!channel->isUserInChannel(inviter->get_nickname())) {
        std::string error_message = ":" + _ServerName + " 442 " + inviter->get_nickname() + " " + channelName + " :You're not on that channel\r\n";
        send(inviter->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        return;
    }
    _chanMap[channelName]->addInviteUser(invitedUser);
    // Send invite message to the invited user
    std::string invite_message = ":" + inviter->get_nickname() + "!" + inviter->get_username() + "@" + inviter->get_hostname() + " INVITE " + invitedUser->get_nickname() + " :" + channelName + "\r\n";
    send(invitedUser->get_fd(), invite_message.c_str(), invite_message.size(), MSG_NOSIGNAL);
    // Notify inviter that the invite was sent
    std::string confirm_message = ":" + _ServerName + " 341 " + inviter->get_nickname() + " " + invitedUser->get_nickname() + " " + channelName + "\r\n";
    send(inviter->get_fd(), confirm_message.c_str(), confirm_message.size(), MSG_NOSIGNAL);
}


void Channel::broadcastMessageToChan(const std::string &message, int sender_fd){
    for (std::map<std::string, Client*>::iterator it = _userMap.begin(); it != _userMap.end(); ++it) {
        Client *client = it->second;
        if (client && client->get_fd() != sender_fd) {
            send(client->get_fd(), message.c_str(), message.size(), MSG_NOSIGNAL);
        }
    }
}


// When joining channel check all modes
bool    Channel::checkAllModes(Client *user, const std::string &nickname, const std::string &password, Server *server)
{
    if (_modeI)
    {
        if (_inviteList.find(nickname) != _inviteList.end())
        {
            _inviteList.erase(nickname);
        }
        else
        {
            std::string inviteOnly = ":" + server->getServerName() + " 473 " + user->get_username() + " " + _chanName + " :Cannot join channel (+i)\r\n"; // 473 ERR_INVITEONLYCHAN
            send(user->get_fd(), inviteOnly.c_str(), inviteOnly.size(), MSG_NOSIGNAL);
            return false;
        }
    }
    if (_modeK)
    {
        if (password != _channelPassword)
        {
            std::string badKey = ":" + server->getServerName() + " 475 " + user->get_username() + " " + _chanName + " :Cannot join channel (+k)\r\n"; // 475 ERR_BADCHANNELKEY
            send(user->get_fd(), badKey.c_str(), badKey.size(), MSG_NOSIGNAL);
            return false;
        }
    }
    if (_modeL)
    {
        if (_nbUsers >= _userLimit)
        {
            std::string userLimit = ":" + server->getServerName() + " 471 " + user->get_username() + " " + _chanName + " :Cannot join channel (+l)\r\n"; // 471 ERR_CHANNELISFULL
            send(user->get_fd(), userLimit.c_str(), userLimit.size(), MSG_NOSIGNAL);
            return false;
        }
    }
    return true;
}




void Server::JOIN(const std::string &chanName, const std::string &nickname, Client *user, const std::string &password) {
    if (_chanMap.find(chanName) == _chanMap.end()) {
        Channel *newChannel = new Channel(chanName);
        _chanMap[chanName] = newChannel;
        _chanMap[chanName]->addUser(user);
        _chanMap[chanName]->grantOperator(user, nickname, this, true);
    } else {
        if (!_chanMap[chanName]->checkAllModes(user, nickname, password, this))
            return;
        _chanMap[chanName]->addUser(user);
    }
    std::string joinMsg = ":" + user->get_nickname() + " JOIN " + chanName + "\r\n";
    send(user->get_fd(), joinMsg.c_str(), joinMsg.size(), MSG_NOSIGNAL);
    // Send RPL messages to the joining user
    _chanMap[chanName]->RPL(user, this, nickname);
    // Broadcast join message to other users in the channel
    _chanMap[chanName]->broadcastMessageToChan(joinMsg, user->get_fd());
    user->insertChannel(chanName);
    user->set_current_channel(chanName);
}
void    Server::LIST(Client *user)
{
    std::map<std::string, Channel*>::iterator it;
    std::string header = "321 Channels :Users Name\r\n"; // RPL 321 list start
    std::string listEnd = ":End of /LIST\r\n";

    send(user->get_fd(), header.c_str(), header.size(), MSG_NOSIGNAL);
    for (it = _chanMap.begin(); it != _chanMap.end(); it++)
    {
        it->second->chanList(user);
    }

    send(user->get_fd(), listEnd.c_str(), listEnd.size(), MSG_NOSIGNAL);
}


/* ------------------------- Channel ------------------------- */

Channel::Channel(const std::string &name) : _nbUsers(0), _userLimit(0), _topicRestriction(false), _isTopic(false), _chanName(name), \
_modeI(false), _modeT(true), _modeK(false), _modeO(false), _modeL(false), _channelPassword("") { }
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
    if (_userMap.find(nickname) != _userMap.end())
    {
        std::string alrOnChannel = ":" + server->getServerName() + " 338 " + nickname + " " + _chanName + ":You are already on that channel\r\n"; // RPL 338 user already on channel
        send(user->get_fd(), alrOnChannel.c_str(), alrOnChannel.size(), MSG_NOSIGNAL);
        return false;
    }
    return true;
}

void    Channel::chanList(Client *user)
{
    std::ostringstream  oss;
    oss << _nbUsers;

    std::string list = "322 " + _chanName + " " + oss.str() + ":" + _topicName + "\r\n"; // RPL 322 list
    send(user->get_fd(), list.c_str(), list.size(), MSG_NOSIGNAL);
}

void    Channel::RPL(Client *user, Server *server, const std::string &nickname)
{
    (void)nickname;
    std::map<std::string, Client*>::iterator it;

    std::string noTopic = ":" + server->getServerName() + " 331 " + _chanName + " :No topic is set\r\n"; // RPL 331 without topic
    std::string topic = ":" + server->getServerName() + " 332 " + user->get_nickname() + " " + _chanName + " :" + _topicName + "\r\n"; // RPL 332 with topic

    std::string namReply = ":" + server->getServerName() + " 353 " + user->get_nickname() + " = " + _chanName + " :"; // RPL 353 list users
    for (std::map<std::string, Client*>::iterator it = _userMap.begin(); it != _userMap.end(); ++it) {
        if (isOps(it->first)) {
            namReply += "@" + it->first + " ";
        } else {
            namReply += it->first + " ";
        }
    }
    namReply += "\r\n";

    std::string endOfName = ":" + server->getServerName() + " 366 " + user->get_nickname() + " " + _chanName + " :End of /NAME list\r\n"; // RPL 366 end of users list

    if (!_isTopic)
        send(user->get_fd(), noTopic.c_str(), noTopic.size(), MSG_NOSIGNAL); // RPL 331
    else
        send(user->get_fd(), topic.c_str(), topic.size(), MSG_NOSIGNAL); // RPL 332
    send(user->get_fd(), namReply.c_str(), namReply.size(), MSG_NOSIGNAL); // RPL 353
    send(user->get_fd(), endOfName.c_str(), endOfName.size(), MSG_NOSIGNAL); // RPL 366
}


  // -------------------> Commands <------------------- //

void    Server::TOPIC(Client *user, const std::string &chanName, const std::string &topicName)
{
    if (_chanMap.find(chanName) != _chanMap.end())
    {
        if (!_chanMap[chanName]->isUserInChannel(user->get_nickname()))
        {
            std::string error_message = ":" + _ServerName + " 442 " + user->get_nickname() + " " + chanName + " :You're not on that channel\r\n";
            send(user->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
            return ;
        }
        _chanMap[chanName]->addTopic(user, this, topicName);
    }
    else
    {
        std::string error_message = ":" + _ServerName + " 403 " + user->get_nickname() + " " + chanName + " :No such channel\r\n";
        send(user->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
    }
}

void    Channel::addTopic(Client *user, Server *server, const std::string &topicName)
{
    if (!topicName.empty())
    {
        if (isOps(user->get_nickname()) || _modeT)
        {
            _topicName = topicName.substr(1);
            _isTopic = true;
            std::string setTopic = ":" + user->get_nickname() + "!" + user->get_nickname() + "@" + user->get_hostname() + " TOPIC " + _chanName + " :" + _topicName + "\r\n";
            send(user->get_fd(), setTopic.c_str(), setTopic.size(), MSG_NOSIGNAL);
            broadcastMessageToChan(setTopic, user->get_fd());
        }
        else
        {
            std::string notOps = ":" + server->getServerName() + " 482 " + user->get_username() + " " + _chanName + " You're not channel operator\r\n";
            send(user->get_fd(), notOps.c_str(), notOps.size(), MSG_NOSIGNAL);
        }
    }
    else
    {
        std::string noTopic = ":" + server->getServerName() + " 331 " + " :No topic is set\r\n"; // RPL 331 without topic
        std::string topic = ":" + server->getServerName() + " 332 " + user->get_nickname() + " " + _chanName + " :" + _topicName + "\r\n"; // RPL 332 with topic
        if (_isTopic)
            send(user->get_fd(), topic.c_str(), topic.size(), MSG_NOSIGNAL);
        else
            send(user->get_fd(), noTopic.c_str(), noTopic.size(), MSG_NOSIGNAL);
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
            send(user->get_fd(), partMsg.c_str(), partMsg.size(), MSG_NOSIGNAL);
            channel->broadcastMessageToChan(partMsg, user->get_fd());
            user->handlePartCommand(chanName);
            channel->eraseUser(user->get_nickname());
            if (channel->getNbUser() == 0) {
                std::cout << "Channel is empty, deleting channel: " << chanName << "\n";
                delete channel;
                _chanMap.erase(it);
            }
        } else { // User is not in the channel, send an error message
            std::string error_message = ":" + _ServerName + " 442 " + user->get_nickname() + " " + chanName + " :You're not on that channel\r\n";
            send(user->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        }
    } else { // Channel does not exist, send an error message
        std::string error_message = ":" + _ServerName + " 403 " + user->get_nickname() + " " + chanName + " :No such channel\r\n";
        send(user->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
    }
}

bool Channel::isEmpty()
{
	if (_userMap.empty())
		return true;
	return false;
}

std::string Client::getID(void)
{
	std::string ID = ":" + this->_nickname + "!" + this->_username + "@" + this->_IPadd;
	return (ID);
}

Client* Server::getClientByFd(int fd) {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_fd() == fd) 
            return &_clients[i];
    }
    return NULL;
}

void Server::PRIVMSG(int senderFd, const std::string &target, const std::string &message) {
    Client *sender = getClientByFd(senderFd);
    if (!sender) {
        std::cerr << "Sender not found for FD: " << senderFd << std::endl;
        return;
    }

    std::string fullMessage;
    size_t pos = message.find(':');
    if (pos != std::string::npos && message.substr(pos + 2, 8) == "DCC SEND") {
        fullMessage = sender->getID() + " PRIVMSG " + target + ":" + message + "\r\n";
    } else
        fullMessage = sender->getID() + " PRIVMSG " + target  + message + "\r\n";
    if (target[0] == '#') {
        // Target is a channel
        Channel *channel = get_Channel(target);
        if (channel) {
            channel->broadcastMessageToChan(fullMessage, senderFd);
        } else {
            std::string errorMessage = ":" + _ServerName + " 403 " + sender->getID() + " " + target + " :No such channel\r\n";
            send(senderFd, errorMessage.c_str(), errorMessage.size(), MSG_NOSIGNAL);
        }
    } else {
        // Target is a specific user
        Client *receiver = findClientByNickname(target);
        if (receiver) {
            send(receiver->get_fd(), fullMessage.c_str(), fullMessage.size(), MSG_NOSIGNAL);
        } else {
            std::string errorMessage = ":" + _ServerName + " 401 " + sender->get_nickname() + " " + target + " :No such nick/channel\r\n";
            send(senderFd, errorMessage.c_str(), errorMessage.size(), MSG_NOSIGNAL);
        }
    }
}


void Server::PASS(Client *client, const std::string &password) {
    if (client->is_authenticated()) {
        std::string error_message = ":" + _ServerName + " 462 " + client->get_nickname() + " :You may not reregister\r\n";
        send(client->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        return;
    }

    if (password == _password) {
        client->authenticate();
        std::string success_message = ":" + _ServerName + " 001 " + client->get_nickname() + " :Password accepted\r\n";
        send(client->get_fd(), success_message.c_str(), success_message.size(), MSG_NOSIGNAL);
    } else {
        std::string error_message = ":" + _ServerName + " 464 " + client->get_nickname() + " :Password incorrect\r\n";
        send(client->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        std::cout << RED << client->get_fd() << " wrong password\r\n" << WHI;
        close(client->get_fd());
        clearClients(client->get_fd());
    }
}

void Channel::KICK(Client *op, Server *server, const std::string &channelName, const std::string &nickname, const std::string &reason) {
    if (!isOps(op->get_nickname())) {
        std::string error_message = ":" + server->getServerName() + " 482 " + op->get_nickname() + " " + channelName + " :You're not channel operator\r\n";
        send(op->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        return;
    }

    if (_userMap.find(nickname) == _userMap.end()) {
        std::string error_message = ":" + server->getServerName() + " 441 " + nickname + " " + channelName + " :They aren't on that channel\r\n";
        send(op->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        return;
    }

    Client *user = _userMap[nickname];
    std::string kick_message = ":" + op->get_nickname() + " KICK " + channelName + " " + nickname + " :" + reason + "\r\n";
    broadcastMessageToChan(kick_message, -1);
    eraseUser(nickname);
    std::string user_message = ":" + server->getServerName() + " 442 " + nickname + " " + channelName + " :" + op->get_nickname() + " kicked you from the channel\r\n";
    send(user->get_fd(), user_message.c_str(), user_message.size(), MSG_NOSIGNAL);
    std::string namreply_message = ":" + server->getServerName() + " 366 " + nickname + " " + channelName + " :End of /NAMES list\r\n";
    send(user->get_fd(), namreply_message.c_str(), namreply_message.size(), MSG_NOSIGNAL);
    std::string confirmation_message = ":" + server->getServerName() + " 369 " + op->get_nickname() + " " + nickname + " :Kicked from " + channelName + "\r\n";
    send(op->get_fd(), confirmation_message.c_str(), confirmation_message.size(), MSG_NOSIGNAL);
}

  // -------------------> Modes <------------------- //

void    Channel::setModes(bool activate, const std::string &mode, Client *user, Server *server)
{
    std::stringstream   ss(mode);
    std::string         password, modes, nickname;

    ss >> modes;
    for (unsigned i = 0; modes[i]; i++)
    {
        if (modes[i] == 'i') {
            handleModeI(activate);
            std::string changeMode = ":" + user->get_username() + " MODE " + _chanName + " " + std::string(activate ? "+" : "-") + "invite only\r\n";
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), MSG_NOSIGNAL);
            broadcastMessageToChan(changeMode, user->get_fd());
        }
        else if (modes[i] == 't') {
            handleModeT(activate);
            std::string changeMode = ":" + user->get_username() + " MODE " + _chanName + " " + std::string(activate ? "+" : "-") + "topic protection\r\n";
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), MSG_NOSIGNAL);
            broadcastMessageToChan(changeMode, user->get_fd());
        }
        else if (modes[i] == 'k') {
            ss >> password;
            handleModeK(activate, password);
            std::string changeMode = ":" + user->get_username() + " MODE " + _chanName + " " + std::string(activate ? "+" : "-") + "password required\r\n";
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), MSG_NOSIGNAL);
            broadcastMessageToChan(changeMode, user->get_fd());
        }
        else if (modes[i] == 'o') {
            ss >> nickname;
            handleModeO(activate, nickname, user, server);
            /*std::string changeMode = ":" + user->get_username() + " MODE " + _chanName + " " + std::string(activate ? ("+o " + nickname + " is now operator") : ("-o " + nickname + " is no longer operator")) + "\r\n";
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), MSG_NOSIGNAL);
            broadcastMessageToChan(changeMode, user->get_fd());*/
        }
        if (modes[i] == 'l') {
            int limit = 0;
            if (activate) {
                ss >> limit;
            }
            std::ostringstream limitStr;
            if (activate) {
                limitStr << limit;
            }
            handleModeL(activate, limit);
            std::string changeMode = ":" + user->get_username() + " MODE " + _chanName + " " + (activate ? "+limit " + limitStr.str() : "-limit") + "\r\n";
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), MSG_NOSIGNAL);
            broadcastMessageToChan(changeMode, user->get_fd());
        }
    }
}

void    Server::MODE(bool activate, const std::string &chanName, const std::string &mode, Client *user)
{
    if (chanName[0] != '#')
    {
        for (size_t i = 0; i < _clients.size(); ++i) {
            if (_clients[i].get_nickname() == chanName) {
                if (mode == "i" && activate)
                {
                    std::string changeMode = ":" + _ServerName + " MODE " + user->get_nickname() + " :+i\r\n";
                    send(user->get_fd(), changeMode.c_str(), changeMode.size(), MSG_NOSIGNAL);
                }
                break;
            }
        }
    }
    else if (_chanMap.find(chanName) != _chanMap.end())
    {
        if (_chanMap[chanName]->isUserInChannel(user->get_nickname()))
        {
            if (_chanMap[chanName]->isOps(user->get_nickname()))
            {
                if (activate)
                    _chanMap[chanName]->setModes(activate, mode, user, this);
                else
                    _chanMap[chanName]->setModes(activate, mode, user, this);
            }
            else
            {
                std::string notOps = ":" + _ServerName + " 482 " + user->get_nickname() + " " + chanName + " :You're not channel operator\r\n";
                send(user->get_fd(), notOps.c_str(), notOps.size(), MSG_NOSIGNAL);
            }
        }
        else
        {
            std::string error_message = ":" + _ServerName + " 442 " + user->get_nickname() + " " + chanName + " :You're not on that channel\r\n";
            send(user->get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        }
    }
    else
    {
        std::cout << "MODE condition\r\n";
        std::string noSuchNick = ":" + _ServerName + " 401 " + user->get_nickname() + " " + chanName + " :No such nick/channel\r\n"; // RPL 401 No such nick
        send(user->get_fd(), noSuchNick.c_str(), noSuchNick.size(), MSG_NOSIGNAL);
    }
}

  // -------------------> Others <------------------- //

bool    Channel::isOps(const std::string &nickname)
{
    if (_userOps.find(nickname) != _userOps.end())
        return true;
    return false;
}

bool    Channel::isUserInChannel(const std::string &nickname) const {
    return _userMap.find(nickname) != _userMap.end();
}

void Channel::changeNicknameInChannel(Client *user, const std::string &old_nickname) {
    if (_userMap.find(old_nickname) != _userMap.end()) {
        _userMap.erase(old_nickname);
        _userMap.insert(std::make_pair(user->get_nickname(), user));
        if (_userOps.find(old_nickname) != _userOps.end()) {
            _userOps.erase(old_nickname);
            _userOps.insert(std::make_pair(user->get_nickname(), user));
        }
        // Broadcast the nickname change to all users in the channel
        std::string nick_change_msg = ":" + old_nickname + "!" + user->get_username() + "@" + user->get_hostname() + " NICK :" + user->get_nickname() + "\r\n";
        broadcastMessageToChan(nick_change_msg, -1);
    } else {
        std::cout << "User is not in Channel: " << _chanName << "\r\n";
    }
}

void Channel::addUser(Client *user) {
    _userMap[user->get_nickname()] = user;
    _nbUsers++;
    std::cout << GRE << "User " << user->get_nickname() << BLU << " added to channel " << _chanName << ". Current users: " << _nbUsers  << WHI<< std::endl;
}

void    Channel::addInviteUser(Client *user)
{
    if (_inviteList.find(user->get_nickname()) == _inviteList.end())
    {
        _inviteList.insert(std::make_pair(user->get_nickname(), user));
    }
}

void    Channel::eraseUser(const std::string &nickname)
{
    if (_userMap.find(nickname) != _userMap.end()){
        _userMap.erase(nickname);
        _nbUsers--;
        std::cout  << GRE << "User " << nickname << BLU << " removed from channel " << _chanName << ". Current users: " << _nbUsers << WHI << std::endl;
    }
}

// Grant / remove operator access
void    Channel::grantOperator(Client *user, const std::string &nickname, Server *server, bool add)
{
    if (add && _userMap.find(nickname) != _userMap.end())
    {
        _userOps.insert(std::make_pair(nickname, user));
        std::string changeMode = ":" + user->get_username() + " MODE " + _chanName + " " + std::string(add ? ("+o " + nickname + " is now operator") : ("-o " + nickname + " is no longer operator")) + "\r\n";
        send(user->get_fd(), changeMode.c_str(), changeMode.size(), MSG_NOSIGNAL);
        broadcastMessageToChan(changeMode, user->get_fd());
    }
    else if (_userOps.find(nickname) != _userOps.end())
    {
        _userOps.erase(nickname);
        std::string changeMode = ":" + user->get_username() + " MODE " + _chanName + " " + std::string(add ? ("+o " + nickname + " is now operator") : ("-o " + nickname + " is no longer operator")) + "\r\n";
        send(user->get_fd(), changeMode.c_str(), changeMode.size(), MSG_NOSIGNAL);
        broadcastMessageToChan(changeMode, user->get_fd());
    }
    else
    {
        std::cout << "Grant ops condition\r\n";
        std::string noSuchNick = ":" + server->getServerName() + " 401 " + user->get_nickname() + " " + _chanName + " :No such nick\r\n"; // RPL 401 No such nick
        send(user->get_fd(), noSuchNick.c_str(), noSuchNick.size(), MSG_NOSIGNAL);
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
    send(fd, pong_response.c_str(), pong_response.size(), MSG_NOSIGNAL);
    std::cout  << GRE << "Sent to " << fd << ": " << WHI << pong_response << std::endl;
}

std::string Server::getUniqueHostname(const std::string &hostname) {
    std::string uniqueHostname = hostname;
    int suffix = 1;

    while (isHostnameInUse(uniqueHostname)) {
        std::ostringstream oss;
        oss << suffix;
        uniqueHostname = hostname + oss.str();
        ++suffix;
    }

    return uniqueHostname;
}

bool Server::isHostnameInUse(const std::string &hostname) {
    for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->get_hostname() == hostname) {
            return true;
        }
    }
    return false;
}

std::string Server::getUniqueUsername(const std::string &username) {
    std::string uniqueUsername = username;
    int suffix = 1;

    while (isUsernameInUse(uniqueUsername)) {
        std::ostringstream oss;
        oss << suffix;
        uniqueUsername = username + oss.str();
        ++suffix;
    }

    return uniqueUsername;
}

bool Server::isUsernameInUse(const std::string &username) {
    for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->get_username() == username) {
            return true;
        }
    }
    return false;
}

Client* Server::findClientByNickname(const std::string& nickname) {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_nickname() == nickname) {
            return &_clients[i];
        }
    }
    return NULL;
}

Channel *Server::get_Channel(const std::string &chanName){
    std::map<std::string, Channel*>::iterator it = _chanMap.find(chanName);
    if (it != _chanMap.end()) {
        return it->second;
    }
    return NULL;
}
