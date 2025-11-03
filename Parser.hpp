#ifndef PARSER_HPP                     // Include guard: evita inclusioni multiple
#define PARSER_HPP

#include <string>                      // Per std::string
#include <vector>                      // Per std::vector
#include <stdexcept>                   // Per lanciare eccezioni in caso di errori di parsing

// Ogni direttiva del file di configurazione è del tipo:
// esempio: "root ./www;" oppure "autoindex on;"
// quindi contiene un nome (root) e una lista di argomenti ("./www")
struct Directive {
	std::string name;                  // Nome della direttiva (es: "root")
	std::vector<std::string> args;     // Argomenti associati (es: ["./www"])
};

// Un blocco rappresenta una sezione delimitata da graffe:
// es: server { ... } o location /path { ... }
// può contenere direttive e sottoblocchi (es. location dentro server)
struct Block {
	std::string type;                  // Tipo del blocco (es: "server" o "location")
	std::string param;                 // Parametro del blocco (es: "/ciao" per location)
	std::vector<Directive> directives; // Tutte le direttive dirette nel blocco
	std::vector<Block> sub_blocks;     // Tutti i sottoblocchi (es. location dentro server)
};

// La classe Parser trasforma la lista di token (prodotta dal Lexer)
// in una gerarchia di oggetti Block (albero di configurazione)
class Parser {
public:
	explicit Parser(const std::vector<std::string>& tokens); // Costruttore: riceve i token dal Lexer
	Block parse();                                           // Avvia il parsing e restituisce il blocco root

private:
	std::vector<std::string> _tokens;   // Lista dei token (es. ["server", "{", "listen", ...])
	size_t _index;                      // Indice attuale nel vettore dei token

	Block parseBlock();                 // Analizza un blocco (es. server { ... })
	Directive parseDirective();         // Analizza una singola direttiva (es. root ./www;)
	std::string peek();                 // Guarda il token corrente senza consumarlo
	std::string get();                  // Prende e avanza al token successivo
};

#endif // Fine include guard
