#include "Client.hpp"

/* -------------- Getter -------------- */
int             get_fd() const { return _fd; }

std::string     get_nickname() const { return _nickname; }


/* -------------- Setter -------------- */
void            set_nickname(std::string new_nickname) { this->_nickname = new_nickname; }

void            set_fd(int fd) { _fd = fd; }

void            set_IPADD(const std::string& ip) { _IPadd = ip; }