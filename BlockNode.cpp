#include "BlockNode.hpp"

BlockNode::BlockNode() {}

BlockNode::BlockNode(const std::string& t, const std::string& p)
    : type(t), param(p) {}

BlockNode::BlockNode(const BlockNode& other)
    : type(other.type),
      param(other.param),
      directives(other.directives),
      sub_blocks(other.sub_blocks) {}

BlockNode& BlockNode::operator=(const BlockNode& other) {
  if (this != &other) {
    type = other.type;
    param = other.param;
    directives = other.directives;
    sub_blocks = other.sub_blocks;
  }
  return *this;
}

BlockNode::~BlockNode() {}
