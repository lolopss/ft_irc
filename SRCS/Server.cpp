#include "Server.hpp"

bool Server::_Signal = false;

Server::Server(char *port, char *password) : _ServerName("PEERC"), _password(password), _Port(atoi(port)), _ServerSocketFd(-1) {}
Server::~Server() {}

void Server::SignalHandler(int signum) {
    (void)signum;
    std::cout << "\nSignal Received!\n";
    Server::_Signal = true;
}

void Server::closeFds() {
    for (size_t i = 0; i < _clients.size(); i++) {
        std::cout << RED << "Client " << _clients[i].get_fd() << " has disconnected\n" << WHI;
        close(_clients[i].get_fd());
    }
    if (_ServerSocketFd != -1) {
        std::cout << RED << "Server " << _ServerSocketFd << " has disconnected\n" << WHI;
        close(_ServerSocketFd);
    }
}

void Server::clearClients(int fd) {
    // Remove from _fds
    for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
        if (it->fd == fd) {
            std::cout << "Removing fd " << fd << " from _fds" << std::endl;
            _fds.erase(it);
            break;
        }
    }
    
    // Remove from _clients
    for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->get_fd() == fd) {
            _clients.erase(it);
            break;
        }
    }
}

void        Server::clearMap()
{
    std::map<std::string, Channel*>::iterator   it;

    for (it = _chanMap.begin(); it != _chanMap.end(); it++)
    {
        //it->second->clearMaps(); <-- clearMaps() est appele dans le destructeur de Channel pour l'instant
        delete it->second;
    }

    _chanMap.clear();
}

void Server::serverSocket() {
    struct sockaddr_in add;
    struct pollfd NewPoll;
    add.sin_family = AF_INET;
    add.sin_port = htons(this->_Port);
    add.sin_addr.s_addr = INADDR_ANY;

    _ServerSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_ServerSocketFd == -1)
        throw(std::runtime_error("failed to create socket"));

    int en = 1;
    if (setsockopt(_ServerSocketFd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en)) == -1)
        throw(std::runtime_error("failed to set option (SO_REUSEADDR) on socket"));
    if (fcntl(_ServerSocketFd, F_SETFL, O_NONBLOCK) == -1)
        throw(std::runtime_error("failed to set option (O_NONBLOCK) on socket"));
    if (bind(_ServerSocketFd, (struct sockaddr *)&add, sizeof(add)) == -1)
        throw(std::runtime_error("failed to bind socket"));
    if (listen(_ServerSocketFd, SOMAXCONN) == -1)
        throw(std::runtime_error("listen() failed"));

    NewPoll.fd = _ServerSocketFd;
    NewPoll.events = POLLIN;
    NewPoll.revents = 0;
    _fds.push_back(NewPoll);
}

void Server::serverInit() {
    this->_client_nb = 0;
    serverSocket();
    std::cout << GRE << "Server <" << _ServerSocketFd << "> Connected <<" << WHI << std::endl;
    std::cout << "Waiting to accept a connection...\n";
}

void Server::sendWelcomeMessages(int client_fd) {
    std::string nickname = getClientNickname(client_fd);

    std::string welcome = ":" + _ServerName + " 001 " + nickname + " :Welcome to the 42RC Network, " + getClientByFd(client_fd)->getID() + "\r\n";
    std::string yourHost = ":" + _ServerName + " 002 " + nickname + " :Your host is " + _ServerName + ", running version 1.0.0\r\n";
    std::string created = ":" + _ServerName + " 003 " + nickname + " :This server was created 2024-06-03\r\n";
    std::string myInfo = ":" + _ServerName + " 004 " + nickname + " " + _ServerName + " 1.0.0 " + "User mode=i Channel modes=itkol" + "\r\n";
    std::string isupport = ":" + _ServerName + " 005 " + nickname + " PREFIX=(ov)@+ CHANTYPES=# CHANMODES=itkol  CASEMAPPING=rfc2119 :are supported by this server\r\n";
    std::string motdStart = ":" + _ServerName + " 375 " + nickname + " :- " + _ServerName + " Message of the day - \r\n";
    std::string motd = ":" + _ServerName + " 372 " + nickname + " :- Welcome to this IRC server!\r\n";
    std::string endOfMotd = ":" + _ServerName + " 376 " + nickname + " :End of /MOTD command.\r\n";

    send(client_fd, welcome.c_str(), welcome.size(), MSG_NOSIGNAL);
    send(client_fd, yourHost.c_str(), yourHost.size(), MSG_NOSIGNAL);
    send(client_fd, created.c_str(), created.size(), MSG_NOSIGNAL);
    send(client_fd, myInfo.c_str(), myInfo.size(), MSG_NOSIGNAL);
    send(client_fd, isupport.c_str(), isupport.size(), MSG_NOSIGNAL);
    send(client_fd, motdStart.c_str(), motdStart.size(), MSG_NOSIGNAL);
    send(client_fd, motd.c_str(), motd.size(), MSG_NOSIGNAL);
    send(client_fd, endOfMotd.c_str(), endOfMotd.size(), MSG_NOSIGNAL);
}

void Server::acceptNewClient() {
    int client_fd = accept(_ServerSocketFd, NULL, NULL);
    if (client_fd == -1) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            std::cerr << "accept() failed" << std::endl;
        }
        return;
    }

    struct pollfd client_poll;
    client_poll.fd = client_fd;
    client_poll.events = POLLIN;
    client_poll.revents = 0;
    _fds.push_back(client_poll);
    Client client;
    client.set_fd(client_fd);
    std::string name = "Guest_";
    std::ostringstream converter;
    converter << client_fd;
    name += converter.str();
    client.set_nickname(name);
    _clients.push_back(client);
    _client_nb++;
    std::cout << "New client connected: " << client_fd << std::endl;
    sendWelcomeMessages(client_fd); // all rpl for start
}

void Server::broadcastMessage(const std::string &message, int sender_fd) {
    if (message.size() < 1)
        return;

    std::string sender_nick;
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_fd() == sender_fd) {
            sender_nick = _clients[i].get_nickname();
            break;
        }
    }
    std::string full_message = sender_nick + ": " + message + "\r\n";
    for (size_t i = 0; i < _clients.size(); ++i) {
        int client_fd = _clients[i].get_fd();
        if (client_fd != sender_fd) {
            size_t total_sent = 0;
            while (total_sent < full_message.size()) {
                size_t chunk_size = std::min(full_message.size() - total_sent, static_cast<size_t>(BUFFER_SIZE - 1));
                int sent = send(client_fd, full_message.c_str() + total_sent, chunk_size, MSG_NOSIGNAL);
                if (sent == -1) {
                    std::cerr << RED << "Failed to send message to client " << client_fd << WHI << std::endl;
                    break;
                }
                total_sent += sent;
            }
            std::cout << "Sent message to client " << client_fd << ": " << full_message << std::endl;
        }
    }
}

std::vector<std::string> Server::split(const std::string &s, char delimiter) { // For multiple channels /join
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool Server::exec_command(std::istringstream &iss, const std::string &command, Client &client, const std::string &msg) {
    
    //std::cout << GRE << "Received command: " << WHI << command << YEL << " from client: " << client.get_fd() << WHI << "\n\n";

    if (command == "CAP") {
        std::string subcommand;
        iss >> subcommand;
        if (subcommand == "LS" || subcommand == "LIST") {
            std::string cap_response = ":" + _ServerName + " CAP * LS :\r\n"; // Respond with supported capabilities
            send(client.get_fd(), cap_response.c_str(), cap_response.size(), MSG_NOSIGNAL);
            return true;
        } else if (subcommand == "END") {
            return true; // CAP negotiation ends
        }
    }
    
    if (!client.is_authenticated()) {
        if (command == "PASS") {
            std::string password;
            iss >> password;
            PASS(&client, password);
            return true;
        } else {
            std::string error_message = ":" + _ServerName + " 464 " + client.get_nickname() + " :Password required\r\n";
            send(client.get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
            std::cout << RED << client.get_fd() << " : No password entered\r\n" << WHI;
            close(client.get_fd());
            clearClients(client.get_fd());
            return false;
        }
    }

    if (command == "NICK") {
        std::string new_nick;
        iss >> new_nick;
        NICK(&client, new_nick);
    } else if (command == "USER") {
        std::string username, hostname, servername, realname;
        iss >> username >> hostname >> servername;
        std::getline(iss, realname);
        USER(&client, username, hostname, servername, realname);
    } else if (command == "PASS") {
        std::string password;
        iss >> password;
        PASS(&client, password);
    }
    else if (command == "WHOIS") {
        std::string target;
        iss >> target;
        WHOIS(&client, target);
    } else if (command == "JOIN") {
    std::string channels, password;
    iss >> channels >> password;
    std::vector<std::string> channel_list = split(channels, ',');
    for (size_t i = 0; i < channel_list.size(); ++i)
        JOIN(channel_list[i], client.get_nickname(), &client, password);
}
    else if (command == "PART") {
        std::string channel_name;
        iss >> channel_name;
        std::string reason;
        std::getline(iss, reason);
        if ((int)reason.find_first_not_of(" :") != -1)
            reason = reason.substr(reason.find_first_not_of(" :"));
        PART(&client, channel_name, reason);
    } else if (command == "LIST") {
        LIST(&client);
    }else if (command == "TOPIC") {
        std::string chanName, topic;
        iss >> chanName;
        if (iss.peek() == ' ') { // Check if there's a space indicating more input
            std::getline(iss, topic);
            if (!topic.empty() && topic[0] == ' ') {
                topic = topic.substr(1); // Remove leading space
            }
        }
        TOPIC(&client, chanName, topic);
    } else if (command == "MODE") {
        std::string mode, chanName, tmp;
        bool        activate = false;
        iss >> chanName;
        iss >> tmp;
        if (!tmp.empty() && (tmp[0] == '+' || tmp[0] == '-'))
        {
            if ((int)msg.find("+") != -1)
            {
                mode = msg.substr(msg.find("+") + 1);
                activate = true;
            }
            else if ((int)msg.find("-") != -1)
            {
                mode = msg.substr(msg.find("-") + 1);
            }
        }
        if (!mode.empty())
            MODE(activate, chanName, mode, &client);
    } else if (command == "PRIVMSG") {
        std::string target, msg;
        iss >> target;
        std::getline(iss, msg);
        PRIVMSG(client.get_fd(), target, msg);
    } else if (command == "PING") {
        std::string msg;
        iss >> msg;
        PING(client.get_fd(), msg);
    } else if (command == "INVITE") {
        std::string nickname, channel_name;
        iss >> nickname >> channel_name;
        INVITE(&client, nickname, channel_name);
    } else if (command == "KICK") {
        std::string channel_name, nickname, reason;
        iss >> channel_name >> nickname;
        std::getline(iss, reason); // Read the rest as reason
        if (!reason.empty() && reason[0] == ' ')
            reason = reason.substr(1); // Remove leading space if present

        Channel *channel = get_Channel(channel_name);
        if (channel)
            channel->KICK(&client, this, channel_name, nickname, reason);
        else {
            std::string error_message = ":" + _ServerName + " 403 " + client.get_nickname() + " " + channel_name + " :No such channel\r\n";
            send(client.get_fd(), error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
        }
    } else {
        return false; // Not a command, handle as a regular message
    }
    return true; // Handled as a command
}

void Server::receiveNewData(int fd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_received <= 0) {
        std::cout << "Client " << fd << " disconnected." << std::endl;
        close(fd);
        clearClients(fd);
        return;
    }

    buffer[bytes_received] = '\0';
    std::string message(buffer);
    _partial_messages[fd] += message;

    size_t pos;
    while ((pos = _partial_messages[fd].find("\r\n")) != std::string::npos) {
        std::string complete_command = _partial_messages[fd].substr(0, pos);
        _partial_messages[fd].erase(0, pos + 2);

        if (!complete_command.empty()) {
            std::cout << GRE << "Received from " << fd << ": " << WHI << complete_command << "\n\n";

            std::istringstream iss(complete_command);
            std::string command;
            iss >> command;

            size_t clientIndex = 0;
            for (size_t i = 0; i < _clients.size(); ++i) {
                if (_clients[i].get_fd() == fd) {
                    clientIndex = i;
                    break;
                }
            }

            Client &client = _clients[clientIndex];
            if (!exec_command(iss, command, client, complete_command)) {
                std::string channelName = client.get_current_channel();
                if (!channelName.empty()) {
                    Channel *channel = get_Channel(channelName);
                    if (channel) {
                        std::string full_message = client.get_nickname() + ": " + complete_command + "\r\n";
                        channel->broadcastMessageToChan(full_message, fd);
                    } else {
                        std::string error_message = ":" + _ServerName + " 401 " + client.get_nickname() + " " + channelName + " :No such channel\r\n";
                        send(fd, error_message.c_str(), error_message.size(), MSG_NOSIGNAL);
                    }
                }
            }
        }
    }
    
}


void Server::run() {
    while (!_Signal) {
        int poll_count = poll(_fds.data(), _fds.size(), -1);
        if (poll_count == -1) {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll() failed");
        }
        for (size_t i = 0; i < _fds.size(); ++i) {
            if (_fds[i].revents & POLLIN) {
                if (_fds[i].fd == _ServerSocketFd) {
                    acceptNewClient();
                } else {
                    receiveNewData(_fds[i].fd);
                }
            }
        }
    }
    clearMap();
    closeFds();
}


/* ------------------ Getters ------------------ */

std::string Server::getServerName() const { return _ServerName; }

std::string Server::getClientNickname(int client_fd) {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_fd() == client_fd) {
            return _clients[i].get_nickname();
        }
    }
    return "";
}
