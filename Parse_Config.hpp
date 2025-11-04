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
	│     └─ vector<string> subparts // split su ':' (es. "0.0.0.0:8080" -> ["0.0.0.0","8080"])
	├─ Internals / helpers
	│  ├─ class ParserState (tokens, idx)  // peek/get/eof
	│  ├─ removeComments(string &)
	│  ├─ tokenize(const string&) -> vector<string>
	│  ├─ splitColon(const string&) -> vector<string>
	│  ├─ parseDirective(ParserState&) -> DirectiveNode
	│  └─ parseBlock(ParserState&) -> BlockNode
	└─ Entry point
	└─ BlockNode parseConfigFile(const std::string &path)  // ritorna root BlockNode

	Esempio (mappatura del tuo webserv.conf):
	root (BlockNode.type="root")
	├─ directives:
	│  ├─ DirectiveNode(name="error_page", args=[ArgPart(raw="404"), ArgPart(raw="/404.html")])
	│  ├─ DirectiveNode(name="error_page", args=[ArgPart(raw="500"), ArgPart(raw="502"), ArgPart(raw="503"), ArgPart(raw="504"), ArgPart(raw="/50x.html")])
	│  └─ DirectiveNode(name="max_request_body", args=[ArgPart(raw="4096")])
	└─ sub_blocks:
	└─ BlockNode.type="server"
		├─ directives:
		│  ├─ DirectiveNode(name="listen", args=[ArgPart(raw="0.0.0.0:8080", subparts=["0.0.0.0","8080"])])
		│  ├─ DirectiveNode(name="root", args=[ArgPart(raw="./www")])
		│  ├─ DirectiveNode(name="autoindex", args=[ArgPart(raw="on")])
		│  ├─ DirectiveNode(name="index", args=[ArgPart(raw="index.html")])
		│  ├─ DirectiveNode(name="upload", args=[ArgPart(raw="off")])
		│  └─ DirectiveNode(name="upload_root", args=[ArgPart(raw="./uploads")])
		└─ sub_blocks:
			├─ BlockNode.type="location", param="/ciao"
			│  └─ directives:
			│     ├─ DirectiveNode(name="method", args=[ArgPart("GET"), ArgPart("POST")])
			│     └─ DirectiveNode(name="root", args=[ArgPart("./ciao")])
			├─ BlockNode.type="location", param="/ciaone"
			│  └─ directives:
			│     └─ DirectiveNode(name="redirect", args=[ArgPart("302"), ArgPart("https://google.com")])
			├─ BlockNode.type="location", param="/upload"
			│  └─ directives:
			│     └─ DirectiveNode(name="upload", args=[ArgPart("on")])
			└─ BlockNode.type="location", param="/cgi"
				└─ directives:
				└─ DirectiveNode(name="cgi", args=[ArgPart("on")])
*/