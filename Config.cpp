#include "Config.hpp"
#include "BlockNode.hpp"
#include "DirectiveNode.hpp"
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

Config::Config() : tokens_(), idx_(0) {}

Config::~Config() {}

Config::Config(const Config &other)
    : tokens_(other.tokens_), idx_(other.idx_) {}

Config &Config::operator=(const Config &other) {
  if (this != &other) {
    tokens_ = other.tokens_;
    idx_ = other.idx_;
  }
  return *this;
}

void Config::removeComments(std::string &s) {
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

void Config::tokenize(const std::string &content) {
  tokens_.clear();
  std::string cur;
  for (size_t i = 0; i < content.size(); ++i) {
    char c = content[i];
    if (c == '{' || c == '}' || c == ';') {
      if (!cur.empty()) {
        tokens_.push_back(cur);
        cur.clear();
      }
      tokens_.push_back(std::string(1, c));
    } else if (std::isspace(static_cast<unsigned char>(c))) {
      if (!cur.empty()) {
        tokens_.push_back(cur);
        cur.clear();
      }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty())
    tokens_.push_back(cur);
  idx_ = 0;
}

bool Config::eof() const {
  return idx_ >= tokens_.size();
}

const std::string &Config::peek() const {
  static std::string empty = "";
  return idx_ < tokens_.size() ? tokens_[idx_] : empty;
}

std::string Config::get() {
  if (idx_ >= tokens_.size())
    throw std::runtime_error("Unexpected end of tokens");
  return tokens_[idx_++];
}

DirectiveNode Config::parseDirective() {
  DirectiveNode d;
  d.name = get();
  while (peek() != ";") {
    if (eof())
      throw std::runtime_error(std::string("Directive '") + d.name +
                               "' missing ';'");
    d.args.push_back(get());
  }
  get(); // consume ;
  return d;
}

BlockNode Config::parseBlock() {
  BlockNode b;
  b.type = get(); // server or location
  if (b.type == "location") {
    if (peek().empty())
      throw std::runtime_error("location missing parameter");
    b.param = get();
  }
  if (get() != "{")
    throw std::runtime_error("Expected '{' after block type");
  while (peek() != "}") {
    if (eof())
      throw std::runtime_error(std::string("Missing '}' for block ") + b.type);
    if (peek() == "location") {
      b.sub_blocks.push_back(parseBlock());
    } else if (peek() == "server") {
      b.sub_blocks.push_back(parseBlock());
    } else {
      b.directives.push_back(parseDirective());
    }
  }
  get(); // consume }
  return b;
}

BlockNode Config::parseFile(const std::string &path) {
  // read file
  std::ifstream file(path.c_str());
  if (!file.is_open())
    throw std::runtime_error(std::string("Unable to open config file: ") +
                             path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  removeComments(content);
  tokenize(content);

  BlockNode root;
  root.type = "root";
  while (!eof()) {
    if (peek() == "server") {
      root.sub_blocks.push_back(parseBlock());
    } else {
      root.directives.push_back(parseDirective());
    }
  }
  return root;
}

// Print Block tree for debugging
// Print BlockNode tree for debugging
static void _printBlockRec(const BlockNode &b, int indent) {
  std::string pad(indent, ' ');
  std::cout << pad << "Block: type='" << b.type << "'";
  if (!b.param.empty())
    std::cout << " param='" << b.param << "'";
  std::cout << "\n";
  for (size_t i = 0; i < b.directives.size(); ++i) {
    const DirectiveNode &d = b.directives[i];
    std::cout << pad << "  Directive: name='" << d.name << "' args=[";
    for (size_t j = 0; j < d.args.size(); ++j) {
      if (j)
        std::cout << ", ";
      std::cout << "'" << d.args[j] << "'";
    }
    std::cout << "]\n";
  }
  for (size_t i = 0; i < b.sub_blocks.size(); ++i)
    _printBlockRec(b.sub_blocks[i], indent + 2);
}

void dumpConfig(const BlockNode &b) {
  _printBlockRec(b, 0);
}
