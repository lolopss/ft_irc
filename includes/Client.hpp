#pragma once

#include "Utility.hpp"

class Client {
    private:
        int                         _fd;
        std::string                 _nickname;
        std::string                 _username;
        std::string                 _hostname;
        std::string                 _servername;
        std::string                 _realname;
        std::string                 _current_channel;
        std::vector<std::string>    _channelList;
        bool                        _registered;
        std::string                 _IPadd; // IP address of the Client 
        bool                        _invisible;
        bool                        _authenticated; // Track if the client is authenticated
        bool                        _isOps;
    public:
        Client();
        ~Client();

        int         get_fd() const;
        void        set_fd(int fd);

        std::string getID(void);

        std::string get_nickname() const;
        void        set_nickname(std::string name);

        std::string get_current_channel()const;
        void        set_current_channel(std::string joined_chan);
    
        std::string get_username() const;
        void        set_username(const std::string &username);

        std::string get_hostname() const;
        void        set_hostname(const std::string &hostname);

        std::string get_servername() const;
        void        set_servername(const std::string &servername);

        std::string get_realname() const;
        void        set_realname(const std::string &realname);

        std::vector<std::string>    get_channelList() const;

        bool        is_registered() const;
        void        set_registered(bool registered);
        
        void        authenticate();
        bool        is_authenticated() const;

        void        set_IPADD(const std::string& ip);
        std::string get_IPADD() const;
        void        handlePartCommand(const std::string &channelName);

        void        insertChannel(const std::string &chanName);
        void        removeChannel(const std::string &chanName);
};