/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ldaniel <ldaniel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/30 14:48:31 by ldaniel           #+#    #+#             */
/*   Updated: 2024/06/17 14:43:55 by ldaniel          ###   ########.fr       */
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
#include <sstream>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <map>
#include "Client.hpp"

#define MAX_MESSAGE_LENGTH 510
#define MSGTOOLONG "Message too long. Maximum length is 510 characters.\r\n"

class Server {
private:
    int                         _Port;
    int                         _ServerSocketFd;
    static bool                 _Signal;
    std::vector<Client>         _clients;
    std::vector<struct pollfd>  _fds;
    int                         _client_nb;

public:
    Server() : _Port(4444), _ServerSocketFd(-1) {}
    ~Server() {}

    std::string getClientNickname(int client_fd);
    void        sendWelcomeMessages(int client_fd);
    void        serverInit();
    void        serverSocket();
    void        acceptNewClient();
    void        receiveNewData(int fd);
    static void SignalHandler(int signum);
    void        closeFds();
    void        clearClients(int fd);
    void        broadcastMessage(const std::string &message, int sender_fd);
    void        run();
    
    
    /*****************Commands(Cmd.cpp)*****************/
    
    void NICK(Client *client, const std::string &new_name);
};

