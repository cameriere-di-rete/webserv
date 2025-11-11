CXX			:=	c++
CXXFLAGS	:=	-Wall -Wextra -Werror -std=c++98

NAME	:=	webserv
SOURCES	:=	main.cpp			\
			BlockNode.cpp		\
			Body.cpp			\
			Config.cpp			\
			Connection.cpp		\
			DirectiveNode.cpp	\
			Header.cpp			\
			Logger.cpp			\
			Message.cpp			\
			Request.cpp			\
			RequestLine.cpp		\
			Response.cpp		\
			Server.cpp			\
			ServerManager.cpp	\
			StatusLine.cpp		\
			utils.cpp		\
			signals.cpp

OBJECTS	:=	$(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS	:=	$(patsubst %.cpp,%.d,$(SOURCES))

# 0 = DEBUG, 1 = INFO, 2 = ERROR
# run `make [...] LOG_LEVEL='N'` to choose the log level
ifneq ($(strip $(LOG_LEVEL)),)
CXXFLAGS += -DLOG_LEVEL=$(LOG_LEVEL)
endif

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

-include $(DEPENDS)

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	$(RM) $(OBJECTS) $(DEPENDS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
