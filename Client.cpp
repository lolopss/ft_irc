#include "Client.hpp"

/* -------------- Getter -------------- */
int             Client::get_fd() const { return _fd; }

std::string     Client::get_nickname() const { return _nickname; }

std::vector<std::string>	Client::get_channelList() { return _channelList; }


/* -------------- Setter -------------- */
void            Client::set_nickname(std::string new_nickname) { this->_nickname = new_nickname; }

void            Client::set_fd(int fd) { _fd = fd; }

void            Client::set_IPADD(const std::string& ip) { _IPadd = ip; }


void			Client::insertChannel(const std::string &chanName)
{
	std::vector<std::string>::iterator	it = std::find(_channelList.begin(), _channelList.end(), chanName);
	if (it == _channelList.end())
		_channelList.push_back(chanName);
}
void			Client::removeChannel(const std::string &chanName)
{
	std::vector<std::string>::iterator	it = std::find(_channelList.begin(), _channelList.end(), chanName);
	if (it != _channelList.end())
		_channelList.erase(it);
}