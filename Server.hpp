/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ldaniel <ldaniel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/30 14:48:31 by ldaniel           #+#    #+#             */
/*   Updated: 2024/06/05 15:02:36 by ldaniel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <vector>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <fcntl.h>
#include "Client.hpp"

class Server {
private:
    int _Port;
    int _ServerSocketFd;
    static bool _Signal;
    std::vector<Client> _clients;
    std::vector<struct pollfd> _fds;

public:
    Server() : _Port(4444), _ServerSocketFd(-1) {}
    void serverInit();
    void serverSocket();
    void acceptNewClient();
    void receiveNewData(int fd);
    static void SignalHandler(int signum);
    void handleNickCommand(int fd, const std::string& new_nickname);
    Client* getClientByFd(int fd);
    void sendMessageToAllClients(int sender_fd, const std::string& message);
    void handleCAPCommand(int fd, const std::string& command);
    void closeFds();
    void clearClients(int fd);
    void run();
    ~Server() {}
};

