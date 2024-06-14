/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ldaniel <ldaniel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/21 15:02:40 by ldaniel           #+#    #+#             */
/*   Updated: 2024/06/14 14:35:40 by ldaniel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"


int main() {
    Server serv;
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