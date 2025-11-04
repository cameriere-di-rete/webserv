// Parse_Config.hpp
#ifndef PARSE_CONFIG_HPP
#define PARSE_CONFIG_HPP

#include <string>
#include <vector>

namespace parsecfg {

// A directive argument part: raw token and optional subparts (split by ':')
struct ArgPart {
  std::string raw;                   // original token
  std::vector<std::string> subparts; // split by ':' if applicable
};

struct DirectiveNode {
  std::string name;          // directive name (first token)
  std::vector<ArgPart> args; // arguments of the directive
};

struct BlockNode {
  std::string type;  // e.g. "server" or "location"
  std::string param; // optional parameter for block (e.g. /path)
  std::vector<DirectiveNode>
      directives;                    // directives directly inside this block
  std::vector<BlockNode> sub_blocks; // nested blocks
};

// Top-level API: parse a file and return the root BlockNode
// Throws std::runtime_error on syntax errors (no semantic validation here)
BlockNode parseConfigFile(const std::string &path);

} // namespace parsecfg

#endif // PARSE_CONFIG_HPP
