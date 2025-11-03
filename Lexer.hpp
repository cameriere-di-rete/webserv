#ifndef LEXER_HPP                 // Include guard: evita che il file venga incluso due volte
#define LEXER_HPP

#include <string>                // Per usare std::string
#include <vector>                // Per restituire un std::vector di token

// Il Lexer legge il contenuto del file di configurazione (come stringa)
// e lo "tokenizza", cioè lo divide in unità sintattiche chiamate token.
// Esempio: "listen 0.0.0.0:8080;" -> ["listen", "0.0.0.0:8080", ";"]
class Lexer {
public:
	// Costruttore: prende il contenuto del file come parametro
	explicit Lexer(const std::string& content);

	// Funzione principale: restituisce un vettore di stringhe (token)
	std::vector<std::string> tokenize();

private:
	std::string _content;        // Salva il contenuto del file internamente

	// Rimuove tutti i commenti dal file.
	// Un commento inizia con '#' e arriva fino alla fine della riga.
	void removeComments();
};

#endif // Fine include guard
