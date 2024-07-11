#pragma once

#include <vector>
#include <poll.h>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <sstream>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <map>
#include <utility> // std::pair / std::make_pair
#include <iterator>
#include <algorithm> // std::find
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>
#include <signal.h>
#include <cerrno>

#define BUFFER_SIZE 1024
#define RED "\e[1;31m"
#define WHI "\e[0;37m"
#define GRE "\e[1;32m"
#define YEL "\e[1;33m"
#define BLU "\e[1;34m"