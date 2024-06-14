/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cmd.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ldaniel <ldaniel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/14 11:33:00 by ldaniel           #+#    #+#             */
/*   Updated: 2024/06/14 14:47:22 by ldaniel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"


void Server::NICK(Client *client, const std::string &new_nick) {
    bool alr_exists = false;
    for (size_t i = 0; i < _clients.size(); i++) {
        if (new_nick == _clients[i].get_nickname()) {
            alr_exists = true;
            std::string buff = new_nick + " already exists on this server.\n";
            std::cerr << buff;
            send(client->get_fd(), buff.c_str(), buff.size(), 0);
            break;
        }
    }
    if (!alr_exists) {
        std::cout << "gskjgnfklsgsfd\n";
        client->set_nickname(new_nick);
        std::string confirmation = "Nickname changed to " + new_nick + "\n";
        send(client->get_fd(), confirmation.c_str(), confirmation.size(), 0);
    }
}