#include "Server.hpp"

bool Server::_Signal = false;

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
    std::string welcome = ":" + _ServerName + " 001 " + getClientNickname(client_fd) + " :Welcome to the IRC server\r\n";
    std::string yourHost = ":" + _ServerName + " 002 " + getClientNickname(client_fd) + " :Your host is PEERC, running version 1.0\r\n";
    std::string created = ":" + _ServerName + " 003 " + getClientNickname(client_fd) + " :This server was created the 3rd of June\r\n";
    std::string myInfo = ":" + _ServerName + " 004 " + getClientNickname(client_fd) + " :PEERC 1.0 o o\r\n";
    std::string motdStart = ":" + _ServerName + " 375 " + getClientNickname(client_fd) + " :- server_name Message of the day - \r\n";
    std::string motd = ":" + _ServerName + " 372 " + getClientNickname(client_fd) + " :- Welcome to this IRC server!\r\n";
    std::string endOfMotd = ":" + _ServerName + " 376 " + getClientNickname(client_fd) + " :End of /MOTD command.\r\n";

    send(client_fd, welcome.c_str(), welcome.size(), 0);
    send(client_fd, yourHost.c_str(), yourHost.size(), 0);
    send(client_fd, created.c_str(), created.size(), 0);
    send(client_fd, myInfo.c_str(), myInfo.size(), 0);
    send(client_fd, motdStart.c_str(), motdStart.size(), 0);
    send(client_fd, motd.c_str(), motd.size(), 0);
    send(client_fd, endOfMotd.c_str(), endOfMotd.size(), 0);
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
                int sent = send(client_fd, full_message.c_str() + total_sent, chunk_size, 0);
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

bool Server::exec_command(std::istringstream &iss, const std::string &command, Client &client, const std::string &msg) {
    if (command == "NICK") {
        std::string new_nick;
        iss >> new_nick;
        NICK(&client, new_nick);
    } else if (command == "USER") {
        std::string username, hostname, servername, realname;
        iss >> username >> hostname >> servername;
        std::getline(iss, realname);
        USER(&client, username, hostname, servername, realname);
    } else if (command == "JOIN") {
        std::string channel_name;
        iss >> channel_name;
        JOIN(channel_name, client.get_nickname(), &client);
    }
    else if (command == "PART") {
        std::string channel_name;
        iss >> channel_name;
        std::string reason;
        std::getline(iss, reason);
        if ((int)reason.find_first_not_of(" :") != -1)
            reason = reason.substr(reason.find_first_not_of(" :"));
        PART(&client, channel_name, reason);
    } else if (command == "LIST" || command == "list") {
        LIST(&client);
    } else if (command == "TOPIC" || command == "topic") {
        std::string chanName, topicName, tmp;
        iss >> chanName;
        iss >> tmp;
        if (!tmp.empty())
        {
            if ((int)msg.find(":") != -1)
            {
                topicName = msg.substr(msg.find(":"));
            }
        }
        if (!chanName.empty())
            TOPIC(&client, chanName, topicName);
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
            std::cout << "Received from " << fd << ": " << complete_command << std::endl;

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
                        std::string error_message = ":server 401 " + client.get_nickname() + " " + channelName + " :No such channel\r\n";
                        send(fd, error_message.c_str(), error_message.size(), 0);
                    }
                }
                // else {
                //     std::string error_message = ":server 404 " + client.get_nickname() + " :You are not in a channel\r\nReal message = " + command + "\r\n";
                //     send(fd, error_message.c_str(), error_message.size(), 0);
                // }
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
