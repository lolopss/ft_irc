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

#include "Cmd.hpp"


void    Server::NICK(Client *client, const std::string &new_nick) {
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

// checker si le channel existe
// rejoindre ou creer le channel
// gerer les cmds et modes

void    Server::JOIN(const std::string &chanName, const std::string &nickname, Client *user)
{
    if (_chanMap.find(chanName) != _chanMap.end())
    {
        _chanMap[chanName]->joinChan(_chanMap[chanName], user, nickname, chanName);
    }
    else
    {
        Channel *newChan = new Channel(chanName);
        _chanMap.insert(std::make_pair(chanName, newChan));
        _chanMap[chanName]->joinChan(_chanMap[chanName], user, nickname, chanName);
    }
}


/* ------------------------- Channel ------------------------- */

Channel::Channel(const std::string &name) : _chanName(name) { }

void    Channel::joinChan(Channel *channel, Client *user, const std::string &nickname, const std::string &chanName)
{
    channel->_userMap.insert(std::make_pair(nickname, user));
    std::cout << nickname << " join " << chanName << "\r\n";
}

std::string Channel::getChanName() const { return _chanName; }