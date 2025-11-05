#include "DirectiveNode.hpp"

DirectiveNode::DirectiveNode() : name(), args() {}

DirectiveNode::DirectiveNode(const std::string &n,
                             const std::vector<std::string> &a)
    : name(n), args(a) {}

DirectiveNode::DirectiveNode(const DirectiveNode &other)
    : name(other.name), args(other.args) {}

DirectiveNode &DirectiveNode::operator=(const DirectiveNode &other) {
  if (this != &other) {
    name = other.name;
    args = other.args;
  }
  return *this;
}

DirectiveNode::~DirectiveNode() {}
