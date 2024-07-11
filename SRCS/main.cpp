#include "Server.hpp"

int check_args(int ac, char **av){
 if (ac != 3){
        std::cout << RED << "./ircserv \"port\"(6690 ~ 6699) \"password\"\n" << WHI;
        return (1);
    }
    if ((atoi(av[1]) < 6690 || atoi(av[1]) > 6699))
    {
        std::cout << RED << "./ircserv \"port\"(6690 ~ 6699) \"password\"\n" << WHI;
        return (2);
    }
    return (0);
}

int main(int ac, char **av) {
    
    if (check_args(ac, av))
        return 1;
    Server serv(av[1], av[2]);
    std::cout << "-----Server-----\n";
    try {
        signal(SIGINT, Server::SignalHandler);
        signal(SIGQUIT, Server::SignalHandler);
        serv.serverInit();
        serv.run();
    } catch (const std::exception &e) {
        serv.closeFds();
        std::cerr << e.what() << std::endl;
    }
    std::cout << "The server closed.\n";
    return 0;
}
