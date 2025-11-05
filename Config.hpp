#pragma once

#include "BlockNode.hpp"
#include "DirectiveNode.hpp"
#include <string>
#include <vector>

// Config is the parser+state holder. It exposes parseFile.
// parseConfigFile(path) is a convenience wrapper.
class Config {
public:
  Config();
  ~Config();
  Config(const Config &other);
  Config &operator=(const Config &other);

  // Parse the file and return the root BlockNode.
  // Throws std::runtime_error on syntax errors.
  BlockNode parseFile(const std::string &path);

private:
  // parser state
  std::vector<std::string> tokens_;
  size_t idx_;

  // helpers
  void removeComments(std::string &s);
  void tokenize(const std::string &content);
  bool eof() const;
  const std::string &peek() const;
  std::string get();

  BlockNode parseBlock();
  DirectiveNode parseDirective();
};

// convenience free function
BlockNode parseConfigFile(const std::string &path);
