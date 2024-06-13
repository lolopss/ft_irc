#include "Client.hpp"

/* -------------- Getter -------------- */
int             Client::get_fd() const { return _fd; }

std::string     Client::get_nickname() const { return _nickname; }


/* -------------- Setter -------------- */
void            Client::set_nickname(std::string new_nickname) { this->_nickname = new_nickname; }

void            Client::set_fd(int fd) { _fd = fd; }

void            Client::set_IPADD(const std::string& ip) { _IPadd = ip; }