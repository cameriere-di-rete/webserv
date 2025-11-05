#pragma once

#include "DirectiveNode.hpp"
#include <string>
#include <vector>

class BlockNode {
public:
  // Orthodox canonical form (C++98)
  BlockNode();
  BlockNode(const std::string &type, const std::string &param = "");
  BlockNode(const BlockNode &other);
  BlockNode &operator=(const BlockNode &other);
  ~BlockNode();

  std::string type;  // e.g. "server" or "location" or "root"
  std::string param; // optional parameter for block (e.g. /path)
  std::vector<DirectiveNode>
      directives;                    // directives directly inside this block
  std::vector<BlockNode> sub_blocks; // nested blocks
};
