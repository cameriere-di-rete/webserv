#include "Lexer.hpp"
#include "Parser.hpp"
#include "ServerManager.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>

int main(void) {
  std::vector<int> ports;

  // Try to read configuration file 'webserv.conf' in current directory
  try {
    std::string content = readFile("webserv.conf");
    Lexer lexer(content);
    std::vector<std::string> tokens = lexer.tokenize();
    Parser parser(tokens);
    Block root = parser.parse();

  // DEBUG: dumpConfig(root);
  dumpConfig(root);

    // Search for server blocks and extract 'listen' directives
    for (size_t i = 0; i < root.sub_blocks.size(); ++i) {
      Block &srv = root.sub_blocks[i];
      if (srv.type == "server") {
        for (size_t j = 0; j < srv.directives.size(); ++j) {
          Directive &d = srv.directives[j];
          if (d.name == "listen" && d.args.size() > 0) {
            // Expect formats like '8080' or '0.0.0.0:8080'
            std::string a = d.args[0];
            size_t pos = a.find(':');
            std::string portstr =
                (pos == std::string::npos) ? a : a.substr(pos + 1);
            int p = std::atoi(portstr.c_str());
            if (p > 0)
              ports.push_back(p);
          }
        }
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Warning: could not read/parse config: " << e.what()
              << "\nFalling back to default ports." << std::endl;
    ports.push_back(8080);
    ports.push_back(9090);
  }

  ServerManager sm;
  try {
    sm.initServers(ports);
  } catch (const std::exception &e) {
    return error(e.what());
  } catch (...) {
    return error("Unknown error while initializing Server");
  }

  /* ignore SIGPIPE so a broken client won't kill the process when we write */
  signal(SIGPIPE, SIG_IGN);

  return sm.run();
}
