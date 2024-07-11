#include "Client.hpp"

Client::Client() : _fd(-1), _nickname(""), _username(""), _hostname(""), _servername(""), _realname(""), _registered(false), _invisible(false), _authenticated(false), _isOps(false) {}
Client::~Client() {}

Client::Client(const Client &o)
{
	_fd = o._fd;
	_nickname = o._nickname;
	_username = o._username;
	_hostname = o._hostname;
	_servername = o._servername;
	_realname = o._realname;
	_current_channel = o._current_channel;
	_channelList = o._channelList;
	_registered = o._registered;
	_IPadd = o._IPadd;
	_invisible = o._invisible;
	_authenticated = o._authenticated;
	_isOps = o._isOps;
}

const Client	Client::operator=(const Client &o)
{
	if (this != &o)
	{
		_fd = o._fd;
		_nickname = o.get_nickname();
		_username = o.get_username();
		_hostname = o.get_hostname();
		_servername = o.get_servername();
		_realname = o.get_realname();
		_current_channel = o.get_current_channel();
		_channelList = o.get_channelList();
		_registered = o.is_registered();
		_IPadd = o.get_IPADD();
		_invisible = o._invisible;
		_authenticated = o.is_authenticated();
		_isOps = o._isOps;
	}

	return *this;
}


/* -------------- Getter -------------- */
std::string	Client::get_nickname() const { return _nickname; }

int	Client::get_fd() const { return _fd; }

std::string Client::get_IPADD() const { return _IPadd; }

std::vector<std::string>	Client::get_channelList() const { return _channelList; }

std::string	Client::get_current_channel()const { return _current_channel; }

std::string Client::get_username() const { return _username; }

std::string	Client::get_hostname() const { return _hostname; }

std::string	Client::get_servername() const { return _servername; }

std::string	Client::get_realname() const { return _realname; }

bool	Client::is_registered() const { return _registered; }

bool	Client::is_authenticated() const { return _authenticated; }

/* -------------- Setter -------------- */
void	Client::set_nickname(std::string new_nickname) { this->_nickname = new_nickname; }

void	Client::set_fd(int fd) { _fd = fd; }

void	Client::set_IPADD(const std::string& ip) { _IPadd = ip; }

void	Client::set_current_channel(std::string joined_chan) { _current_channel = joined_chan; }

void	Client::set_username(const std::string &username) { _username = username; }

void	Client::set_hostname(const std::string &hostname) { _hostname = hostname; }

void	Client::set_servername(const std::string &servername) { _servername = servername; }

void	Client::set_realname(const std::string &realname) { _realname = realname; }

void	Client::set_registered(bool registered) { _registered = registered; }

void	Client::authenticate() { _authenticated = true; }


void	Client::insertChannel(const std::string &chanName)
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
