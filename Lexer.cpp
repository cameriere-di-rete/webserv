#include "Lexer.hpp"             // Include il file header per avere la dichiarazione della classe
#include <sstream>               // Per gestire stringhe facilmente
#include <cctype>                // Per usare funzioni come isspace()
#include <iostream>              // Solo per debug (puoi rimuoverlo in produzione)


// Costruttore: inizializza la variabile privata _content con il testo del file
Lexer::Lexer(const std::string& content) : _content(content) {}


// Funzione privata: rimuove tutti i commenti dal contenuto.
// I commenti in webserv iniziano con '#' e terminano con il carattere di newline '\n'.
void Lexer::removeComments() {
	size_t pos = 0;                                  // Posizione attuale nella stringa

	// Cicla finché trova un carattere '#' nel contenuto
	while ((pos = _content.find('#', pos)) != std::string::npos) {
		size_t end = _content.find('\n', pos);       // Trova la fine della riga (dove finisce il commento)
		if (end == std::string::npos)                // Se non c’è un newline dopo '#'
			_content.erase(pos);                     // Rimuove tutto fino alla fine
		else
			_content.erase(pos, end - pos);          // Rimuove dal '#' fino al newline
	}
}


// Funzione principale: converte il contenuto del file in token
std::vector<std::string> Lexer::tokenize() {
	removeComments();                                // Prima rimuove eventuali commenti

	std::vector<std::string> tokens;                 // Lista dei token da restituire
	std::string current;                             // Buffer temporaneo per costruire il token attuale

	// Scorre ogni carattere del file
	for (size_t i = 0; i < _content.size(); ++i) {
		char c = _content[i];                        // Carattere corrente

		// Caso 1: caratteri speciali che formano token a sé stanti
		if (c == '{' || c == '}' || c == ';') {
			// Se stavamo costruendo un token (es: "listen"), lo salviamo prima
			if (!current.empty()) {
				tokens.push_back(current);
				current.clear();                     // Svuotiamo il buffer per il prossimo token
			}
			// Aggiungiamo il carattere come token separato (es: "{")
			tokens.push_back(std::string(1, c));
		}

		// Caso 2: spazi, tab o newline — separano i token
		else if (isspace(c)) {
			// Se stavamo leggendo un token, lo aggiungiamo
			if (!current.empty()) {
				tokens.push_back(current);
				current.clear();
			}
			// Gli spazi di per sé non vengono aggiunti ai token
		}

		// Caso 3: parte di un token normale (lettera, numero, ecc.)
		else {
			current += c;                             // Accumula il carattere nel token corrente
		}
	}

	// Se il file finisce mentre stiamo leggendo un token, lo aggiungiamo
	if (!current.empty())
		tokens.push_back(current);

	// Restituisce la lista finale dei token
	return tokens;
}
