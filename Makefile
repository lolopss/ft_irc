NAME = ft_irc
CC = c++
FLAG = -Wall -Wextra -Werror -std=c++98 -g3
SRCS = 	SRCS/main.cpp			\
		SRCS/Client.cpp		\
		SRCS/Server.cpp		\
		SRCS/Commands.cpp	\
		SRCS/Mode.cpp
OBJS = $(SRCS:.cpp=.o)
INCLUDE = -I./includes

.cpp.o	:
	$(CC) $(FLAG) $(INCLUDE) -c $< -o $(<:.cpp=.o)

all : $(NAME)

$(NAME) : $(OBJS)
	$(CC) $(FLAG) -o $(NAME) $(OBJS)

clean :
	$(RM) $(OBJS)

fclean : clean
	$(RM) $(NAME)

re : fclean all

test: re
	terminator -e "./ft_irc 6690 oui" -p --title="ft_irc Server" &
	sleep 1
	terminator -e "irssi -c localhost -p 6690 -n user1 -w oui" -p --title="irssi Client b" &
	sleep 1
	terminator -e "irssi -c localhost -p 6690 -n user2 -w oui" -p --title="irssi Client a" &
