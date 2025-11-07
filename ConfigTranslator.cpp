#include "ConfigTranslator.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>

ConfigTranslator::ConfigTranslator() {}
ConfigTranslator::ConfigTranslator(const ConfigTranslator &other) {
  (void)other;
}
ConfigTranslator &ConfigTranslator::operator=(const ConfigTranslator &other) {
  (void)other;
  return *this;
}
ConfigTranslator::~ConfigTranslator() {}

int ConfigTranslator::parsePort(const std::string &listen_arg) {
  std::string portstr;
  size_t colon_pos = listen_arg.find(':');
  if (colon_pos != std::string::npos) {
    portstr = listen_arg.substr(colon_pos + 1);
  } else {
    portstr = listen_arg;
  }
  return std::atoi(portstr.c_str());
}

std::string ConfigTranslator::parseHost(const std::string &listen_arg) {
  size_t colon_pos = listen_arg.find(':');
  if (colon_pos != std::string::npos) {
    return listen_arg.substr(0, colon_pos);
  }
  return "0.0.0.0";
}

bool ConfigTranslator::parseBool(const std::string &value) {
  return (value == "on" || value == "true" || value == "1");
}

std::vector<Server> ConfigTranslator::translateConfig(
    const BlockNode &root, std::map<int, std::string> &global_error_pages,
    std::size_t &global_max_request_body) {
  std::vector<Server> servers;

  // Parse global directives
  global_max_request_body = 0;
  global_error_pages.clear();

  for (size_t i = 0; i < root.directives.size(); ++i) {
    const DirectiveNode &d = root.directives[i];

    if (d.name == "error_page" && d.args.size() >= 2) {
      std::string path = d.args[d.args.size() - 1];
      for (size_t j = 0; j < d.args.size() - 1; ++j) {
        int code = std::atoi(d.args[j].c_str());
        if (code > 0) {
          global_error_pages[code] = path;
        }
      }
    } else if (d.name == "max_request_body" && d.args.size() >= 1) {
      global_max_request_body =
          static_cast<std::size_t>(std::atol(d.args[0].c_str()));
    }
  }

  // Parse server blocks
  for (size_t i = 0; i < root.sub_blocks.size(); ++i) {
    const BlockNode &block = root.sub_blocks[i];
    if (block.type == "server") {
      Server srv = translateServerBlock(block, global_error_pages,
                                        global_max_request_body);
      servers.push_back(srv);
    }
  }

  return servers;
}

Server ConfigTranslator::translateServerBlock(
    const BlockNode &server_block,
    const std::map<int, std::string> &global_error_pages,
    std::size_t global_max_request_body) {
  Server srv;

  // Parse listen directive first to get port
  for (size_t i = 0; i < server_block.directives.size(); ++i) {
    const DirectiveNode &d = server_block.directives[i];
    if (d.name == "listen" && d.args.size() >= 1) {
      srv.port = parsePort(d.args[0]);
      srv.host = parseHost(d.args[0]);
      break;
    }
  }

  // Store all directives in the map
  for (size_t i = 0; i < server_block.directives.size(); ++i) {
    const DirectiveNode &d = server_block.directives[i];
    srv.directives[d.name] = d.args;
  }

  // Add global error_pages if not overridden
  if (srv.directives.find("error_page") == srv.directives.end()) {
    std::vector<std::string> error_args;
    for (std::map<int, std::string>::const_iterator it =
             global_error_pages.begin();
         it != global_error_pages.end(); ++it) {
      std::ostringstream oss;
      oss << it->first;
      error_args.push_back(oss.str());
      error_args.push_back(it->second);
    }
    if (!error_args.empty()) {
      srv.directives["error_page"] = error_args;
    }
  }

  // Add global max_request_body if not overridden
  if (srv.directives.find("max_request_body") == srv.directives.end() &&
      global_max_request_body > 0) {
    std::vector<std::string> body_args;
    std::ostringstream oss;
    oss << global_max_request_body;
    body_args.push_back(oss.str());
    srv.directives["max_request_body"] = body_args;
  }

  // Parse location blocks
  for (size_t i = 0; i < server_block.sub_blocks.size(); ++i) {
    const BlockNode &block = server_block.sub_blocks[i];
    if (block.type == "location") {
      Location loc = translateLocationBlock(block);
      srv.locations[loc.path] = loc;
    }
  }

  return srv;
}

Location
ConfigTranslator::translateLocationBlock(const BlockNode &location_block) {
  Location loc(location_block.param);

  // Store all directives in the map
  for (size_t i = 0; i < location_block.directives.size(); ++i) {
    const DirectiveNode &d = location_block.directives[i];
    loc.directives[d.name] = d.args;
  }

  return loc;
}
