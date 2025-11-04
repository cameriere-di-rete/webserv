#ifndef PARSECFG_DIRECTIVE_NODE_HPP
#define PARSECFG_DIRECTIVE_NODE_HPP

#include <string>
#include <vector>

namespace parsecfg {

class DirectiveNode {
public:
  // Orthodox canonical form (C++98)
  DirectiveNode();
  DirectiveNode(const std::string &name, const std::vector<std::string> &args);
  DirectiveNode(const DirectiveNode &other);
  DirectiveNode &operator=(const DirectiveNode &other);
  ~DirectiveNode();

  std::string name;
  std::vector<std::string> args; // raw argument tokens
};

} // namespace parsecfg

#endif // PARSECFG_DIRECTIVE_NODE_HPP
