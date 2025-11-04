// Parse_Config.cpp
#include "Parse_Config.hpp"
#include "utils.hpp"
#include <cctype>
#include <stdexcept>

namespace parsecfg {

// Helper: remove comments '#' until end of line
static void removeComments(std::string &s) {
  size_t pos = 0;
  while ((pos = s.find('#', pos)) != std::string::npos) {
    size_t e = s.find('\n', pos);
    if (e == std::string::npos) {
      s.erase(pos);
      break;
    }
    s.erase(pos, e - pos);
  }
}

// Tokenize: split into tokens, keeping '{', '}', ';' as separate tokens
static std::vector<std::string> tokenize(const std::string &content) {
  std::vector<std::string> tokens;
  std::string cur;
  for (size_t i = 0; i < content.size(); ++i) {
    char c = content[i];
    if (c == '{' || c == '}' || c == ';') {
      if (!cur.empty()) {
        tokens.push_back(cur);
        cur.clear();
      }
      tokens.push_back(std::string(1, c));
    } else if (std::isspace(static_cast<unsigned char>(c))) {
      if (!cur.empty()) {
        tokens.push_back(cur);
        cur.clear();
      }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty())
    tokens.push_back(cur);
  return tokens;
}

// Split an argument token by ':' into subparts
static std::vector<std::string> splitColon(const std::string &tok) {
  std::vector<std::string> out;
  std::string cur;
  for (size_t i = 0; i < tok.size(); ++i) {
    if (tok[i] == ':') {
      out.push_back(cur);
      cur.clear();
    } else
      cur.push_back(tok[i]);
  }
  out.push_back(cur);
  return out;
}

class ParserState {
public:
  ParserState(const std::vector<std::string> &t) : tokens(t), idx(0) {}
  const std::string &peek() const {
    static std::string empty = "";
    return idx < tokens.size() ? tokens[idx] : empty;
  }
  std::string get() {
    if (idx >= tokens.size())
      throw std::runtime_error("Unexpected end of tokens");
    return tokens[idx++];
  }
  bool eof() const {
    return idx >= tokens.size();
  }

private:
  const std::vector<std::string> &tokens;
  size_t idx;
};

// Forward
static BlockNode parseBlock(ParserState &s);

static DirectiveNode parseDirective(ParserState &s) {
  DirectiveNode d;
  d.name = s.get();
  while (s.peek() != ";") {
    if (s.eof())
      throw std::runtime_error(std::string("Directive '") + d.name +
                               "' missing ';'");
    std::string a = s.get();
    ArgPart p;
    p.raw = a;
    p.subparts = splitColon(a);
    d.args.push_back(p);
  }
  s.get(); // consume ;
  return d;
}

static BlockNode parseBlock(ParserState &s) {
  BlockNode b;
  b.type = s.get(); // server or location
  if (b.type == "location") {
    if (s.peek().empty())
      throw std::runtime_error("location missing parameter");
    b.param = s.get();
  }
  if (s.get() != "{")
    throw std::runtime_error("Expected '{' after block type");
  while (s.peek() != "}") {
    if (s.eof())
      throw std::runtime_error(std::string("Missing '}' for block ") + b.type);
    if (s.peek() == "location") {
      b.sub_blocks.push_back(parseBlock(s));
    } else {
      b.directives.push_back(parseDirective(s));
    }
  }
  s.get(); // consume }
  return b;
}

BlockNode parseConfigFile(const std::string &path) {
  std::string content = readFile(path);
  removeComments(content);
  std::vector<std::string> tokens = tokenize(content);

  ParserState s(tokens);
  BlockNode root;
  root.type = "root";
  while (!s.eof()) {
    if (s.peek() == "server") {
      root.sub_blocks.push_back(parseBlock(s));
    } else {
      // global directives
      root.directives.push_back(parseDirective(s));
    }
  }
  return root;
}

} // namespace parsecfg
