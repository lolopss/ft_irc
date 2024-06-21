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
    if (message.size() <= 1)
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

int Server::exec_command(std::istringstream &iss, std::string &command, Client &client, int &fd){
    if (command == "/NICK" || command == "/nick") {
        std::string new_nick;
        iss >> new_nick;
        if (!new_nick.empty()) {
            NICK(&client, new_nick);
        }
    }
    else if (command == "/JOIN" || command == "/join") {
        //std::vector<std::string>    chanName;
        std::string                 TchanName; iss >> TchanName;
        //while (iss >> TchanName)
        //{
            //chanName.insert(TchanName);
            if (TchanName.empty())
                return 0;
        //}
        JOIN(TchanName, client.get_nickname(), &client);
    }
    else if (command == "/PING" || command == "/ping"){
        std::string msg;
        iss >> msg;
        PING(&client, msg);
    }
    else if (command == "/PART" || command == "/part"){
        std::string chanName;
        std::string reason;
        iss >> chanName;
        iss >> reason;
        PART(&client, chanName, reason);
    }
    else if (command == "/PRIVMSG" || command == "/privmsg") {
        std::string target;
        iss >> target;
        std::string private_message;
        std::getline(iss, private_message);
        private_message = private_message.substr(1); // Remove ":" from message
        PRIVMSG(fd, target, private_message);
    }
    else
        return 0;
    return 1;
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

    // Vérifiez si nous avons une commande complète (délimitée par "\r\n")
    size_t pos;
    while ((pos = _partial_messages[fd].find("\r\n")) != std::string::npos) {
        // Extraire la commande complète
        std::string complete_command = _partial_messages[fd].substr(0, pos);
        _partial_messages[fd].erase(0, pos + 2); // Supprimez aussi le "\r\n"

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

            if (!exec_command(iss, command, _clients[clientIndex], fd)) { // if there's no command, then send message
                broadcastMessage(complete_command, fd);
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
