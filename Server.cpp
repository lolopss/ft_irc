/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ldaniel <ldaniel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/22 15:09:19 by ldaniel           #+#    #+#             */
/*   Updated: 2024/06/17 13:29:31 by ldaniel          ###   ########.fr       */
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
    this->_client_nb = 0;
    serverSocket();
    std::cout << GRE << "Server <" << _ServerSocketFd << "> Connected <<" << WHI << std::endl;
    std::cout << "Waiting to accept a connection...\n";
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
}

void Server::broadcastMessage(const std::string &message, int sender_fd) {
    if (message.size() == 1)
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
                    // Handle send error
                    break;
                }
                total_sent += sent;
            }
        }
    }
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

    buffer[bytes_received] = '\0'; // Null-terminate the received string
    std::string message(buffer);

    if (message.size() != 1) {
        std::cout << "Received: " << message;
    }

    std::istringstream iss(message);
    std::string command;
    iss >> command;

    if (command == "/NICK") {
        std::string new_nick;
        iss >> new_nick;
        if (!new_nick.empty()) {
            for (size_t i = 0; i < _clients.size(); ++i) {
                if (_clients[i].get_fd() == fd) {
                    NICK(&_clients[i], new_nick);
                    break;
                }
            }
        }
    }
    else if (command == "/JOIN") {
        std::string chanName;
        iss >> chanName;
        std::cout << "chan name is " << chanName << "\r\n";
        for (size_t i = 0; i < _clients.size(); ++i) {
            if (_clients[i].get_fd() == fd) {
                JOIN(chanName, _clients[i].get_nickname(), &_clients[i]);
                break;
            }
        }
    }
    else {
        broadcastMessage(message, fd);
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
    closeFds();
}
