#include "Parser.hpp" // Include il file header del parser
#include <cctype>
#include <cstdlib>

// C++98 helper functions (no lambdas / initializer_list)
static bool isNumberString(const std::string &s) {
  if (s.empty())
    return false;
  for (size_t i = 0; i < s.size(); ++i)
    if (!std::isdigit(static_cast<unsigned char>(s[i])))
      return false;
  return true;
}

static bool isOnOffString(const std::string &v) {
  return (v == "on" || v == "off");
}

static bool isAllowedMethod(const std::string &m) {
  const char *allowed[] = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS"};
  for (size_t i = 0; i < sizeof(allowed) / sizeof(allowed[0]); ++i)
    if (m == allowed[i])
      return true;
  return false;
}

// Basic path validation: non-empty, no spaces, starts with '/', './' or alnum
static bool isValidPath(const std::string &p) {
  if (p.empty())
    return false;
  for (size_t i = 0; i < p.size(); ++i)
    if (std::isspace(static_cast<unsigned char>(p[i])))
      return false;
  if (p[0] == '/' || p[0] == '.')
    return true;
  if (std::isalnum(static_cast<unsigned char>(p[0])))
    return true;
  return false;
}

// helper: check if string contains whitespace
static bool containsSpace(const std::string &s) {
  for (size_t i = 0; i < s.size(); ++i)
    if (std::isspace(static_cast<unsigned char>(s[i])))
      return true;
  return false;
}

// Costruttore: salva i token e imposta l'indice iniziale a 0
Parser::Parser(const std::vector<std::string> &tokens)
    : _tokens(tokens), _index(0) {}

// peek() restituisce il token corrente SENZA avanzare l'indice.
// Se siamo alla fine della lista, restituisce una stringa vuota.
std::string Parser::peek() {
  if (_index >= _tokens.size()) // Se abbiamo superato l'ultimo token
    return "";                  // Restituiamo una stringa vuota
  return _tokens[_index];       // Altrimenti restituiamo il token corrente
}

// get() restituisce il token corrente e AVANZA al successivo.
// Se non ci sono più token, lancia un errore.
std::string Parser::get() {
  if (_index >= _tokens.size()) // Controllo di sicurezza
    throw std::runtime_error("Fine inattesa dei token");
  return _tokens[_index++]; // Restituisce il token e incrementa l’indice
}

// parse() è il punto d'ingresso principale del parser.
// Qui viene creato il "blocco radice" che conterrà tutto il file.
Block Parser::parse() {
  Block root;         // Crea un blocco principale che conterrà tutto
  root.type = "root"; // Tipo "root" per distinguere il blocco base

  // Scorri tutti i token
  while (_index < _tokens.size()) {
    // Se il token è "server", significa che inizia un blocco server { ... }
    if (peek() == "server")
      root.sub_blocks.push_back(parseBlock()); // Analizza tutto il blocco e
                                               // aggiungilo come sotto-blocco
    else
      // Altrimenti è una direttiva globale (es. error_page, max_request_body)
      root.directives.push_back(parseDirective());
  }
  return root; // Restituisce il blocco completo (l’albero di configurazione)
}

// parseBlock() viene chiamato quando incontriamo un blocco tipo "server" o
// "location"
Block Parser::parseBlock() {
  Block block;        // Crea un nuovo blocco
  block.type = get(); // Legge il tipo (es. "server" o "location")

  // Se è un blocco location, il prossimo token è il percorso (es. "/ciao")
  if (block.type == "location")
    block.param = get(); // Salva "/ciao" in block.param

  // Controlla che il prossimo token sia "{" altrimenti errore di sintassi
  if (get() != "{")
    throw std::runtime_error("Errore di sintassi: manca '{' dopo " +
                             block.type);

  // Finché non troviamo "}", leggiamo le direttive o sottoblocchi
  while (peek() != "}") {
    if (peek().empty()) // Se i token finiscono prima della '}', è un errore
      throw std::runtime_error("Errore: manca '}' di chiusura per " +
                               block.type);

    if (peek() == "location") // Se troviamo "location", è un sottoblocco
      block.sub_blocks.push_back(parseBlock());
    else
      // Altrimenti è una normale direttiva
      block.directives.push_back(parseDirective());
  }

  get();        // Consuma il token '}' finale
  return block; // Restituisce il blocco completato
}

// parseDirective() viene chiamato quando troviamo una riga come:
// root ./www;  oppure  autoindex on;
Directive Parser::parseDirective() {
  Directive dir;    // Crea una nuova direttiva
  dir.name = get(); // Il primo token è il nome (es. "root")

  // Finché non troviamo ';', leggiamo tutti gli argomenti
  while (peek() != ";") {
    if (peek().empty()) // Se finisce il file prima del ';', errore
      throw std::runtime_error("Errore di sintassi: direttiva senza ';' dopo " +
                               dir.name);
    dir.args.push_back(get()); // Aggiungiamo l’argomento (es. "./www")
  }

  get(); // Consuma il ';' finale

  // ------------ Validation checks for known directives ------------
  const std::string name = dir.name;

  if (name == "listen") {
    if (dir.args.size() != 1)
      throw std::runtime_error("listen: expected exactly one argument");
    std::string a = dir.args[0];
    size_t pos = a.find(':');
    std::string portstr = (pos == std::string::npos) ? a : a.substr(pos + 1);
    if (!isNumberString(portstr))
      throw std::runtime_error("listen: invalid port '" + portstr + "'");
    long p = std::labs(std::atol(portstr.c_str()));
    if (p <= 0 || p > 65535)
      throw std::runtime_error("listen: port out of range: " + portstr);
  } else if (name == "autoindex" || name == "upload" || name == "cgi") {
    if (dir.args.size() != 1)
      throw std::runtime_error(name +
                               ": expected exactly one argument 'on' or 'off'");
    if (!isOnOffString(dir.args[0]))
      throw std::runtime_error(name + ": expected 'on' or 'off', got '" +
                               dir.args[0] + "'");
  } else if (name == "max_request_body") {
    if (dir.args.size() != 1 || !isNumberString(dir.args[0]))
      throw std::runtime_error(
          "max_request_body: expected a single numeric value");
  } else if (name == "error_page") {
    if (dir.args.size() < 2)
      throw std::runtime_error(
          "error_page: expected at least one code and a path");
    for (size_t i = 0; i + 1 < dir.args.size(); ++i) {
      if (!isNumberString(dir.args[i]) || dir.args[i].size() != 3)
        throw std::runtime_error("error_page: invalid status code '" +
                                 dir.args[i] + "'");
    }
    // last arg should be a path (basic check)
    if (dir.args.back().empty())
      throw std::runtime_error("error_page: invalid path");
  } else if (name == "method") {
    if (dir.args.empty())
      throw std::runtime_error("method: expected at least one HTTP method");
    for (size_t i = 0; i < dir.args.size(); ++i) {
      if (!isAllowedMethod(dir.args[i]))
        throw std::runtime_error(std::string("method: unsupported method '") +
                                 dir.args[i] + "'");
    }
  } else if (name == "redirect") {
    if (dir.args.size() < 2)
      throw std::runtime_error("redirect: expected status code and target URL");
    if (!isNumberString(dir.args[0]))
      throw std::runtime_error("redirect: invalid status code '" + dir.args[0] +
                               "'");
    int code = std::atoi(dir.args[0].c_str());
    if (code < 300 || code > 399)
      throw std::runtime_error("redirect: status code must be 3xx");
  }

  // Additional path/index validations
  if (name == "root") {
    if (dir.args.size() != 1 || !isValidPath(dir.args[0]))
      throw std::runtime_error("root: expected a valid path");
  } else if (name == "upload_root") {
    if (dir.args.size() != 1 || !isValidPath(dir.args[0]))
      throw std::runtime_error("upload_root: expected a valid path");
  } else if (name == "index") {
    if (dir.args.empty())
      throw std::runtime_error("index: expected at least one filename");
    for (size_t i = 0; i < dir.args.size(); ++i) {
      if (dir.args[i].empty() || containsSpace(dir.args[i]))
        throw std::runtime_error("index: invalid filename '");
    }
  }

  return dir; // Restituisce la direttiva completata
}
