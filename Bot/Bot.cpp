#include "Bot.hpp"

Bot::Bot(const std::string& server, int port, const std::string& password)
    : _server(server), _port(port), _password(password), _nickname("IRCBot") {}

Bot::~Bot() {}

void* startListening(void* arg) {
    Bot* bot = static_cast<Bot*>(arg);
    bot->_listen();
    return NULL;
}

void* startSendingMessages(void* arg) {
    Bot* bot = static_cast<Bot*>(arg);
    bot->_sendMessageLoop();
    return NULL;
}

void    Bot::connect() {
    struct sockaddr_in server_addr;
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);
    inet_pton(AF_INET, _server.c_str(), &server_addr.sin_addr);

    if (::connect(_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting" << std::endl;
        exit(1);
    }

    _authenticate();
    _sendCommand("JOIN #bot");
    pthread_t listen_thread, send_thread;
    pthread_create(&listen_thread, NULL, startListening, this);
    pthread_detach(listen_thread);
    pthread_create(&send_thread, NULL, startSendingMessages, this);
    pthread_detach(send_thread);
}

void    Bot::_authenticate() {
    _sendCommand("PASS " + _password);
    _sendCommand("NICK " + _nickname);
    _sendCommand("USER " + _nickname + " 0 * :" + _nickname);
}

void    Bot::_sendCommand(const std::string& command) {
    std::string msg = command + "\r\n";
    send(_sockfd, msg.c_str(), msg.size(), 0);
}

void    Bot::_listen() {
    char buffer[512];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(_sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break;

        std::string message(buffer, bytes_received);
        _handleServerMessage(message);
    }
}

void    Bot::_handleServerMessage(const std::string& message) {
    std::istringstream iss(message);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("PING") == 0) {
            _sendCommand("PONG " + line.substr(5));
        } else if (line.find("PRIVMSG") != std::string::npos) {
            size_t sender_end = line.find('!');
            if (sender_end != std::string::npos) {
                std::string from = line.substr(1, sender_end - 1);
                size_t target_start = line.find("PRIVMSG") + 8;
                size_t target_end = line.find(':', target_start);
                std::string target = line.substr(target_start, target_end - target_start);
                std::string msg = line.substr(target_end + 1);
                
                // Remove leading colon from msg if present
                if (!msg.empty() && msg[0] == ':') {
                    msg = msg.substr(1);
                }

                _handleCommand(from, target, msg);
            }
        }
    }
}

void    Bot::_handleCommand(const std::string& from, const std::string& target, const std::string& msg) {
    std::string trimmedMsg = msg;
    if (!trimmedMsg.empty() && trimmedMsg[0] == ' ' && trimmedMsg.size() > 1) {
        trimmedMsg = trimmedMsg.substr(1); // Remove leading colon
        if (!trimmedMsg.empty() && trimmedMsg[0] == ':') {
            trimmedMsg = trimmedMsg.substr(1); // Remove leading space if present
        }
    }
    
    if (trimmedMsg.find("!help") == 0) {
        _sendCommand("PRIVMSG " + target + "Hello " + from + ", I am your friendly IRC bot! I wont be of any help today, except !time");
    } else if (trimmedMsg.find("!time") == 0) {
        time_t now = time(NULL);
        _sendCommand("PRIVMSG " + target + "Current time: " + std::string(ctime(&now)));
    }
}
void    Bot::_sendMessageLoop() {
    while (true) {
        _sendCommand("PRIVMSG #bot :Hello, I am a bot! !time | !help");
        sleep(600); // Sleep for 10 minutes
    }
}

int     main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server> <port> <password>" << std::endl;
        return 1;
    }
    std::string server = argv[1];
    int port = std::atoi(argv[2]);
    std::string password = argv[3];

    Bot bot(server, port, password);
    bot.connect();
    while (true)
        sleep(1);
    return 0;
}
