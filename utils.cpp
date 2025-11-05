#include "BlockNode.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>

int error(const std::string &s) {
  std::cerr << s << ": " << strerror(errno) << std::endl;
  return EXIT_FAILURE;
}

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Print Block tree for debugging
// Print BlockNode tree for debugging
static void _printBlockRec(const BlockNode &b, int indent) {
  std::string pad(indent, ' ');
  std::cout << pad << "Block: type='" << b.type << "'";
  if (!b.param.empty())
    std::cout << " param='" << b.param << "'";
  std::cout << "\n";
  for (size_t i = 0; i < b.directives.size(); ++i) {
    const DirectiveNode &d = b.directives[i];
    std::cout << pad << "  Directive: name='" << d.name << "' args=[";
    for (size_t j = 0; j < d.args.size(); ++j) {
      if (j)
        std::cout << ", ";
      std::cout << "'" << d.args[j] << "'";
    }
    std::cout << "]\n";
  }
  for (size_t i = 0; i < b.sub_blocks.size(); ++i)
    _printBlockRec(b.sub_blocks[i], indent + 2);
}

void dumpConfig(const BlockNode &b) {
  _printBlockRec(b, 0);
}
