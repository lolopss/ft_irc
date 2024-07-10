#include "Channel.hpp"

void    Server::USER(Client *client, const std::string &username, const std::string &hostname, const std::string &servername, const std::string &realname) {
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
    std::string whois = user->get_nickname() + "\r\n" + user->get_username() + "\r\n" + user->get_IPADD() + "\r\n" + (user->get_fd() != -1 ? "connected\r\n" : "not connected\r\n");
    std::cout << "whois size = " << whois.size() << " || " << "WHOIS var is equal to : " << whois.c_str();
    send(client->get_fd(), whois.c_str(), whois.size(), 0);
}

void    Server::NICK(Client *client, const std::string &new_nick) {
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
    // function change nickname in channel
    std::cout << "channellist size = " << client->get_channelList().size() << "\r\n";
    for (unsigned i = 0; i < client->get_channelList().size(); i++)
    {
        _chanMap[client->get_channelList()[i]]->changeNicknameInChannel(client, old_nick);
    }
    std::string confirmation = ":server 001 " + new_nick + " :Nickname changed to " + new_nick + "\r\n";
    std::cout << "Client " << client->get_fd() << " changed nickname from " << old_nick << " to " << new_nick << "\n";
    send(client->get_fd(), confirmation.c_str(), confirmation.size(), 0);
}

Client* Server::findClientByNickname(const std::string& nickname) {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_nickname() == nickname) {
            return &_clients[i];
        }
    }
    return NULL;
}

void Server::INVITE(Client *inviter, const std::string &nickname, const std::string &channelName) {
    // Find the invited user
    Client *invitedUser = findClientByNickname(nickname);
    if (!invitedUser) {
        std::string error_message = ":server 401 " + inviter->get_nickname() + " " + nickname + " :No such nick/channel\r\n";
        send(inviter->get_fd(), error_message.c_str(), error_message.size(), 0);
        return;
    }

    // Find the channel
    std::map<std::string, Channel*>::iterator it = _chanMap.find(channelName);
    if (it == _chanMap.end()) {
        std::string error_message = ":server 403 " + inviter->get_nickname() + " " + channelName + " :No such channel\r\n";
        send(inviter->get_fd(), error_message.c_str(), error_message.size(), 0);
        return;
    }
    Channel *channel = it->second;

    // Check if inviter is in the channel
    if (!channel->isUserInChannel(inviter->get_nickname())) {
        std::string error_message = ":server 442 " + inviter->get_nickname() + " " + channelName + " :You're not on that channel\r\n";
        send(inviter->get_fd(), error_message.c_str(), error_message.size(), 0);
        return;
    }
    _chanMap[channelName]->addInviteUser(invitedUser);
    // Send invite message to the invited user
    std::string invite_message = ":" + inviter->get_nickname() + "!" + inviter->get_username() + "@" + inviter->get_hostname() + " INVITE " + invitedUser->get_nickname() + " :" + channelName + "\r\n";
    send(invitedUser->get_fd(), invite_message.c_str(), invite_message.size(), 0);
    // Notify inviter that the invite was sent
    std::string confirm_message = ":server 341 " + inviter->get_nickname() + " " + invitedUser->get_nickname() + " " + channelName + "\r\n";
    send(inviter->get_fd(), confirm_message.c_str(), confirm_message.size(), 0);
}

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

bool    Channel::checkAllModes(Client *user, const std::string &nickname, const std::string &password)
{
    if (_modeI)
    {
        if (_inviteList.find(nickname) != _inviteList.end())
        {
            _inviteList.erase(nickname);
        }
        else
        {
            std::string inviteOnly = _chanName + " :Invite only, can't join channel without an invitation\r\n"; // 473 ERR_INVITEONLYCHAN
            send(user->get_fd(), inviteOnly.c_str(), inviteOnly.size(), 0);
            return false;
        }
    }
    if (_modeK)
    {
        if (password != _channelPassword)
        {
            std::string badKey = _chanName + " :Bad key input, can't join channel\r\n"; // 475 ERR_BADCHANNELKEY
            send(user->get_fd(), badKey.c_str(), badKey.size(), 0);
            return false;
        }
    }
    if (_modeL)
    {
        if (_nbUsers == _userLimit)
        {
            std::string userLimit = _chanName + " :User limit reach, can't join channel\r\n"; // 471 ERR_CHANNELISFULL
            send(user->get_fd(), userLimit.c_str(), userLimit.size(), 0);
            return false;
        }
    }
    return true;
}

void    Server::JOIN(const std::string &chanName, const std::string &nickname, Client *user, const std::string &password)
{
    if (_chanMap.find(chanName) == _chanMap.end()) {
        Channel *newChannel = new Channel(chanName);
        _chanMap[chanName] = newChannel;
        _chanMap[chanName]->addUser(user);
        _chanMap[chanName]->grantOperator(user, nickname, this, true);
    }
    else
    {
        if (!_chanMap[chanName]->checkAllModes(user, nickname, password))
            return ;
        _chanMap[chanName]->addUser(user);
    }
    std::string joinMsg = ":" + user->get_nickname() + " JOIN :" + chanName + "\r\n";
    send(user->get_fd(), joinMsg.c_str(), joinMsg.size(), 0);
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

    send(user->get_fd(), header.c_str(), header.size(), 0);
    for (it = _chanMap.begin(); it != _chanMap.end(); it++)
    {
        it->second->chanList(user);
    }

    send(user->get_fd(), listEnd.c_str(), listEnd.size(), 0);
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
    (void)nickname;
    std::map<std::string, Client*>::iterator it;

    std::string noTopic = _chanName + " :No topic is set\r\n"; // RPL 331 without topic
    std::string topic = _chanName + " :" + _topicName + "\r\n"; // RPL 332 with topic

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
    (void)server;
    if (!topicName.empty())
    {
        std::cout << "In condition topic is empty\r\n";
        if ((_modeT && isOps(user->get_nickname())) || !_modeT)
        {
            _topicName = topicName.substr(1);
            std::string setTopic = ":" + user->get_nickname() + "!" + user->get_nickname() + "@localhost TOPIC " + _chanName + " :" + _topicName + "\r\n";
            std::cout << "Topic is : " << setTopic << "\r\n";
            send(user->get_fd(), setTopic.c_str(), setTopic.size(), 0);
        }
        else
        {
            std::string notOps = _chanName + " :" + user->get_nickname() + " is not operator of that channel\r\n";
            send(user->get_fd(), notOps.c_str(), notOps.size(), 0);
        }
    }
    else
    {
        std::cout << "In condition topic is NOT empty\r\n";
        std::string topic = _chanName + (_topicName.empty() ? " :No topic is set" : _topicName) + "\r\n";
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
                std::cout << "Channel is empty, deleting channel: " << chanName << "\n";
                delete channel;
                _chanMap.erase(it);
            }
        } else { // User is not in the channel, send an error message
            std::string error_message = ":server 442 " + user->get_nickname() + " " + chanName + " :You're not on that channel\r\n";
            send(user->get_fd(), error_message.c_str(), error_message.size(), 0);
        }
    } else { // Channel does not exist, send an error message
        std::string error_message = ":server 403 " + user->get_nickname() + " " + chanName + " :No such channel\r\n";
        send(user->get_fd(), error_message.c_str(), error_message.size(), 0);
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
        fullMessage = sender->getID() + " PRIVMSG " + target + " :" + message + "\r\n";
    if (target[0] == '#') {
        // Target is a channel
        Channel *channel = get_Channel(target);
        if (channel) {
            channel->broadcastMessageToChan(fullMessage, senderFd);
        } else {
            std::string errorMessage = ":server 403 " + sender->getID() + " " + target + " :No such channel\r\n";
            send(senderFd, errorMessage.c_str(), errorMessage.size(), 0);
        }
    } else {
        // Target is a specific user
        Client *receiver = findClientByNickname(target);
        if (receiver) {
            std::cout << "FULL MESSAGE = " << fullMessage << "END OF THE FULL MESSAGE.\r\n";
            send(receiver->get_fd(), fullMessage.c_str(), fullMessage.size(), 0);
        } else {
            std::string errorMessage = ":server 401 " + sender->get_nickname() + " " + target + " :No such nick/channel\r\n";
            send(senderFd, errorMessage.c_str(), errorMessage.size(), 0);
        }
    }
}


void Server::PASS(Client *client, const std::string &password) {
    if (client->is_authenticated()) {
        std::string error_message = ":server 462 " + client->get_nickname() + " :You may not reregister\r\n";
        send(client->get_fd(), error_message.c_str(), error_message.size(), 0);
        return;
    }

    if (password == _password) {
        client->authenticate();
        std::string success_message = ":server 001 " + client->get_nickname() + " :Password accepted\r\n";
        send(client->get_fd(), success_message.c_str(), success_message.size(), 0);
    } else {
        std::string error_message = ":server 464 " + client->get_nickname() + " :Password incorrect\r\n";
        send(client->get_fd(), error_message.c_str(), error_message.size(), 0);
        std::cout << RED << client->get_fd() << " wrong password\r\n" << WHI;
        close(client->get_fd());
        clearClients(client->get_fd());
    }
}

void Channel::KICK(Client *op, const std::string &channelName, const std::string &nickname, const std::string &reason) {
    if (!isOps(op->get_nickname())) {
        std::string error_message = ":server 482 " + op->get_nickname() + " " + channelName + " :You're not channel operator\r\n";
        send(op->get_fd(), error_message.c_str(), error_message.size(), 0);
        return;
    }

    if (_userMap.find(nickname) == _userMap.end()) {
        std::string error_message = ":server 441 " + nickname + " " + channelName + " :They aren't on that channel\r\n";
        send(op->get_fd(), error_message.c_str(), error_message.size(), 0);
        return;
    }

    Client *user = _userMap[nickname];
    std::string kick_message = ":" + op->get_nickname() + " KICK " + channelName + " " + nickname + " :" + reason + "\r\n";
    broadcastMessageToChan(kick_message, -1);
    std::string user_message = ":server 442 " + nickname + " " + channelName + " :" + op->get_nickname() + " kicked you from the channel\r\n";
    send(user->get_fd(), user_message.c_str(), user_message.size(), 0);
    eraseUser(nickname);
    std::string confirmation_message = ":server 369 " + op->get_nickname() + " " + nickname + " :Kicked from " + channelName + "\r\n";
    send(op->get_fd(), confirmation_message.c_str(), confirmation_message.size(), 0);
}

  // -------------------> Modes <------------------- //

void    Channel::setModes(bool activate, const std::string &mode, Client *user, Server *server)
{
    std::stringstream   ss(mode);
    std::string         password, modes, nickname;
    int                 limit;

    ss >> modes;
    std::cout << "in condition\r\n";
    for (unsigned i = 0; modes[i]; i++)
    {
        if (modes[i] == 'i') {
            handleModeI(activate);
            std::string changeMode = std::string(activate ? "\'+\'" : "\'-\'") + " i : turn invite mode " + (activate ? "on\r\n" : "off\r\n");
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), 0);
        }
        else if (modes[i] == 't') {
            handleModeT(activate);
            std::string changeMode = std::string(activate ? "\'+\'" : "\'-\'") + " t : " + (activate ? "add" : "remove") + " topic restriction\r\n";
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), 0);
        }
        else if (modes[i] == 'k') {
            ss >> password;
            handleModeK(activate, password);
            std::string changeMode = std::string(activate ? "\'+\'" : "\'-\'") + " k : " + (activate ? "change" : "remove") + " channel key\r\n";
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), 0);
        }
        else if (modes[i] == 'o') {
            ss >> nickname;
            handleModeO(activate, nickname, user, server);
            std::string changeMode = std::string(activate ? "\'+\'" : "\'-\'") + " o : " + nickname + (activate ? " is now channel operator\r\n" : " is no more channel operator\r\n");
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), 0);
        }
        else if (modes[i] == 'l') {
            ss >> limit;
            std::ostringstream  limitStr;
            limitStr << limit;
            handleModeL(activate, limit);
            std::string changeMode = std::string(activate ? "\'+\'" : "\'-\'") + " l : " + (activate ? ("channel user limit is now " + limitStr.str() + "\r\n") : "remove user limit\r\n");
            send(user->get_fd(), changeMode.c_str(), changeMode.size(), 0);
        }
    }
}

void    Server::MODE(bool activate, const std::string &chanName, const std::string &mode, Client *user)
{
    std::cout << "| in mode function |\r\n";
    if (chanName[0] != '#')
    {
        for (size_t i = 0; i < _clients.size(); ++i) {
            if (_clients[i].get_nickname() == chanName) {
                if (mode == "+i")
                    std::cout << "client is invisible\r\n";
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
                std::string notOps = chanName + " :" + user->get_nickname() + " is not operator of that channel\r\n";
                send(user->get_fd(), notOps.c_str(), notOps.size(), 0);
            }
        }
        else
        {
            std::string error_message = ":" + _ServerName + " 442 " + user->get_nickname() + " " + chanName + " :You're not on that channel\r\n";
            send(user->get_fd(), error_message.c_str(), error_message.size(), 0);
        }
    }
    else
    {
        std::string noSuchNick = ":" + _ServerName + " 401 " + user->get_nickname() + " " + chanName + " :No such nick/channel\r\n"; // RPL 401 No such nick
        send(user->get_fd(), noSuchNick.c_str(), noSuchNick.size(), 0);
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

void    Channel::changeNicknameInChannel(Client *user, const std::string &nickname)
{

    if (_userMap.find(nickname) != _userMap.end())
    {
        _userMap.erase(nickname);
        _userMap.insert(std::make_pair(user->get_nickname(), user));
        if (_userOps.find(nickname) != _userOps.end())
        {
            _userOps.erase(nickname);
            _userOps.insert(std::make_pair(user->get_nickname(), user));
        }
    }
    else
    {
        std::cout << "user is not in Channel : " << _chanName << "\r\n";
    }

}

void Channel::addUser(Client *user) {
    _userMap[user->get_nickname()] = user;
    _nbUsers++;
    std::cout << "User " << user->get_nickname() << " added to channel " << _chanName << ". Current users: " << _nbUsers << std::endl;
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
    std::cout << "Sent to " << fd << ": " << pong_response;
}
