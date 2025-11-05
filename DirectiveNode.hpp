#pragma once

#include <string>
#include <vector>

class DirectiveNode {
public:
  DirectiveNode();
  DirectiveNode(const std::string &name, const std::vector<std::string> &args);
  DirectiveNode(const DirectiveNode &other);
  DirectiveNode &operator=(const DirectiveNode &other);
  ~DirectiveNode();

  std::string name;
  std::vector<std::string> args; // raw argument tokens
};
