NAME = ft_irc
CC = c++
FLAG = -Wall -Wextra -Werror -std=c++98 -g3
SRCS = main.cpp			\
		Client.cpp		\
		Server.cpp		\
		Commands.cpp	\
		Mode.cpp	
OBJS = $(SRCS:.cpp=.o)
INCLUDE = -I.

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
