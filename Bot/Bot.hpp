#pragma once

#include "Server.hpp"
#include <string>

class Bot {
    private:
        std::string _server;
        int _port;
        std::string _password;
        std::string _nickname;
        int _sockfd;

    public:
        Bot(const std::string& server, int port, const std::string& password);
        ~Bot();
        void connect();
        void _authenticate();
        void _listen();
        void _sendCommand(const std::string& command);
        void _handleServerMessage(const std::string& message);
        void _handleCommand(const std::string& from, const std::string& to, const std::string& msg);
        void _sendMessageLoop();
};
