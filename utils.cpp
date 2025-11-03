#include "Parser.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>

int error(const char *s) {
  std::cerr << s << ": " << strerror(errno) << std::endl;
  return EXIT_FAILURE;
}

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Implementazione della funzione per leggere file di configurazione
std::string readFile(const std::string &path) {
  std::ifstream file(path.c_str());
  if (!file.is_open())
    throw std::runtime_error(
        std::string("Impossibile aprire il file di configurazione: ") + path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Print Block tree for debugging
static void _printBlockRec(const Block &b, int indent) {
  std::string pad(indent, ' ');
  std::cout << pad << "Block: type='" << b.type << "'";
  if (!b.param.empty())
    std::cout << " param='" << b.param << "'";
  std::cout << "\n";
  for (size_t i = 0; i < b.directives.size(); ++i) {
    const Directive &d = b.directives[i];
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

void dumpConfig(const Block &b) {
  _printBlockRec(b, 0);
}
