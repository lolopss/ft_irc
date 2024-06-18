/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cmd.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ldaniel <ldaniel@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/14 11:32:55 by ldaniel           #+#    #+#             */
/*   Updated: 2024/06/14 11:39:08 by ldaniel          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Server.hpp"

class Server;

class Channel {
private:
    bool        _isTopic;
    std::string _topicName;
    std::string _chanName;
    // container map pour avoir les donnees de chaque utilisateurs
    std::map<std::string, Client*> _userMap;
public:
    Channel();
    Channel(const std::string &name);
    ~Channel();

    std::string getChanName() const;
    void    	joinChan(Server *server, Client *user, const std::string &nickname, const std::string &chanName);
    void        RPL(Client *user, Server *server);
};

    //JOIN()
    //create()
    //join()
    //cmd()