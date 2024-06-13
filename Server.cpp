/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ldaniel <ldaniel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/22 15:09:19 by ldaniel           #+#    #+#             */
/*   Updated: 2024/06/05 15:29:15 by ldaniel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
    for (size_t i = 0; i < _fds.size(); i++) {
        if (_fds[i].fd == fd) {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].get_fd() == fd) {
            _clients.erase(_clients.begin() + i);
            break;
        }
    }
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
    this->_Port = 4444;
    serverSocket();
    std::cout << GRE << "Server <" << _ServerSocketFd << "> Connected <<" << WHI << std::endl;
    std::cout << "Waiting to accept a connection...\n";
}


void Server::receiveNewData(int fd) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(fd, buffer, BUFFER_SIZE - 1, 0);
    std::cout << "Buffer = " << buffer << "\n";
    if (bytes_received <= 0) {
        std::cout << "Client " << fd << " disconnected." << std::endl;
        close(fd);
        clearClients(fd);
    } else {
        std::cout << "Received: " << buffer << std::endl;

        // Parse the message
        std::string input(buffer);
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "CAP") {
            handleCAPCommand(fd, input);
        } else if (command == "PRIVMSG") {
            std::string recipient;
            iss >> recipient;
            size_t message_start = input.find(" :");
            std::string message = input.substr(message_start + 2);
            message = ":" + getClientByFd(fd)->get_nickname() + " PRIVMSG " + recipient + " :" + message + "\r\n";
            sendMessageToAllClients(fd, message);
        } else if (command == "NICK") {
            std::string new_nickname;
            iss >> new_nickname;
            handleNickCommand(fd, new_nickname);
        }
        // Other IRC commands can be handled here
    }
}


void Server::handleCAPCommand(int fd, const std::string& command) {
    std::istringstream iss(command);
    std::string cap, subcommand;
    iss >> cap >> subcommand; // Skip CAP part and get the actual subcommand

    if (subcommand == "LS") {
        // Respond with an empty list of supported capabilities
        std::string response = "CAP * LS :\r\n";
        send(fd, response.c_str(), response.size(), 0);
    } else if (subcommand == "REQ") {
        // For simplicity, deny any requested capabilities
        std::string response = "CAP * NAK :\r\n";
        send(fd, response.c_str(), response.size(), 0);
    } else if (subcommand == "END") {
        // CAP END means the capability negotiation is complete
        // No specific response needed for CAP END
    }
}

Client* Server::getClientByFd(int fd) {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_fd() == fd) {
            return &_clients[i];
        }
    }
    return NULL;
}

void Server::handleNickCommand(int fd, const std::string& new_nickname) {
    Client* client = getClientByFd(fd);
    if (client == NULL) {
        return;
    }

    std::string old_nickname = client->get_nickname();
    client->set_nickname(new_nickname);

    // Format the NICK change message
    std::string message = ":" + old_nickname + " NICK :" + new_nickname + "\r\n";

    // Broadcast the NICK change to all clients
    for (size_t i = 0; i < _clients.size(); ++i) {
        send(_clients[i].get_fd(), message.c_str(), message.size(), 0);
    }
}

void Server::sendMessageToAllClients(int sender_fd, const std::string& message) {
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].get_fd() != sender_fd) {
            send(_clients[i].get_fd(), message.c_str(), message.size(), 0);
        }
    }
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

    // Convert client_fd to a string for the nickname
    std::ostringstream oss;
    oss << "Guest" << client_fd;
    client.set_nickname(oss.str());

    _clients.push_back(client);

    std::cout << "New client connected: " << client_fd << std::endl;
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
    closeFds();
}
