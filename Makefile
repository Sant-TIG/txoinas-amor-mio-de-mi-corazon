NAME = webserv

SRCS = main.cpp \
	   Config.cpp \
	   Utils.cpp \
	   Server.cpp \
	   WebServer.cpp \
	   Location.cpp \
	   http_tcpServer_linux.cpp \


OBJS = $(SRCS:.cpp=.o)

C = c++

CFLAGS = -Wall -Wextra -Werror -std=c++98

RM = rm -rf

all: $(NAME)

$(OBJS): %.o: %.cpp
	$(C) $(CFLAGS) -c $< -o $@

$(NAME) : $(OBJS)
	$(C) $(CFLAGS) $(OBJS) -o $(NAME)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: 	fclean all

.PHONY: all clean fclean re