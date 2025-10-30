CXX			:=	c++
CXXFLAGS	:=	-Wall -Wextra -Werror -std=c++98

NAME	:=	webserv
SOURCES	:=	main.cpp Body.cpp Header.cpp Message.cpp Request.cpp RequestLine.cpp Response.cpp StatusLine.cpp

OBJECTS	:=	$(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS	:=	$(patsubst %.cpp,%.d,$(SOURCES))

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
