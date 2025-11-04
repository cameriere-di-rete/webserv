// Parse_Config.hpp
#ifndef PARSE_CONFIG_HPP
#define PARSE_CONFIG_HPP

#include "BlockNode.hpp"
#include "DirectiveNode.hpp"
#include <string>
#include <vector>

namespace parsecfg {

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

} // namespace parsecfg

#endif // PARSE_CONFIG_HPP

/*

treemap del config attuale:

        parsecfg (namespace)
        ├─ Types / AST
        │  ├─ BlockNode
        │  │  ├─ string type           // es. "root", "server", "location"
        │  │  ├─ string param          // es. "/path" per location (opzionale)
        │  │  ├─ vector<DirectiveNode> directives
        │  │  └─ vector<BlockNode> sub_blocks
        │  ├─ DirectiveNode
        │  │  ├─ string name           // es. "listen", "root", "index"
        │  │  └─ vector<ArgPart> args
        │  └─ ArgPart
        │     ├─ string raw            // argomento così com'è nel file
        │     └─ vector<string> subparts // split su ':' (es. "0.0.0.0:8080" ->
["0.0.0.0","8080"]) ├─ Internals / helpers │  ├─ class ParserState (tokens, idx)
// peek/get/eof │  ├─ removeComments(string &) │  ├─ tokenize(const string&) ->
vector<string> │  ├─ splitColon(const string&) -> vector<string> │  ├─
parseDirective(ParserState&) -> DirectiveNode │  └─ parseBlock(ParserState&) ->
BlockNode └─ Entry point └─ BlockNode parseConfigFile(const std::string &path)
// ritorna root BlockNode

        Esempio (mappatura del tuo webserv.conf):
        root (BlockNode.type="root")
        ├─ directives:
        │  ├─ DirectiveNode(name="error_page", args=[ArgPart(raw="404"),
ArgPart(raw="/404.html")]) │  ├─ DirectiveNode(name="error_page",
args=[ArgPart(raw="500"), ArgPart(raw="502"), ArgPart(raw="503"),
ArgPart(raw="504"), ArgPart(raw="/50x.html")]) │  └─
DirectiveNode(name="max_request_body", args=[ArgPart(raw="4096")]) └─
sub_blocks: └─ BlockNode.type="server" ├─ directives: │  ├─
DirectiveNode(name="listen", args=[ArgPart(raw="0.0.0.0:8080",
subparts=["0.0.0.0","8080"])]) │  ├─ DirectiveNode(name="root",
args=[ArgPart(raw="./www")]) │  ├─ DirectiveNode(name="autoindex",
args=[ArgPart(raw="on")]) │  ├─ DirectiveNode(name="index",
args=[ArgPart(raw="index.html")]) │  ├─ DirectiveNode(name="upload",
args=[ArgPart(raw="off")]) │  └─ DirectiveNode(name="upload_root",
args=[ArgPart(raw="./uploads")]) └─ sub_blocks: ├─ BlockNode.type="location",
param="/ciao" │  └─ directives: │     ├─ DirectiveNode(name="method",
args=[ArgPart("GET"), ArgPart("POST")]) │     └─ DirectiveNode(name="root",
args=[ArgPart("./ciao")]) ├─ BlockNode.type="location", param="/ciaone" │  └─
directives: │     └─ DirectiveNode(name="redirect", args=[ArgPart("302"),
ArgPart("https://google.com")]) ├─ BlockNode.type="location", param="/upload" │
└─ directives: │     └─ DirectiveNode(name="upload", args=[ArgPart("on")]) └─
BlockNode.type="location", param="/cgi" └─ directives: └─
DirectiveNode(name="cgi", args=[ArgPart("on")])
*/