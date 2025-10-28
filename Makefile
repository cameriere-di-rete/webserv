CXX			=	c++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++98

NAME	=	webserv
FILES	=	main.cpp Server.cpp
HEADERS	= constants.hpp Server.hpp

OBJ		=	$(FILES:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
