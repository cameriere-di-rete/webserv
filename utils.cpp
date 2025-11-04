#include "Parse_Config.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>

// Print an error message in red to stderr
void printError(const std::string &msg) {
  const char *RED = "\x1b[31m";
  const char *RESET = "\x1b[0m";
  std::cerr << RED << msg << RESET << std::endl;
}

int error(const char *s) {
  std::string m = std::string(s) + ": " + strerror(errno);
  printError(m);
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
// Print parsecfg::BlockNode tree for debugging
static void _printBlockRec(const parsecfg::BlockNode &b, int indent) {
  std::string pad(indent, ' ');
  std::cout << pad << "Block: type='" << b.type << "'";
  if (!b.param.empty())
    std::cout << " param='" << b.param << "'";
  std::cout << "\n";
  for (size_t i = 0; i < b.directives.size(); ++i) {
    const parsecfg::DirectiveNode &d = b.directives[i];
    std::cout << pad << "  Directive: name='" << d.name << "' args=[";
    for (size_t j = 0; j < d.args.size(); ++j) {
      if (j)
        std::cout << ", ";
      std::cout << "'" << d.args[j].raw << "'";
      if (!d.args[j].subparts.empty()) {
        std::cout << "{";
        for (size_t k = 0; k < d.args[j].subparts.size(); ++k) {
          if (k)
            std::cout << ",";
          std::cout << d.args[j].subparts[k];
        }
        std::cout << "}";
      }
    }
    std::cout << "]\n";
  }
  for (size_t i = 0; i < b.sub_blocks.size(); ++i)
    _printBlockRec(b.sub_blocks[i], indent + 2);
}

void dumpConfig(const parsecfg::BlockNode &b) {
  _printBlockRec(b, 0);
}
