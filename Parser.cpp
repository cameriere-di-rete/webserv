#include "Parser.hpp"                  // Include il file header del parser

// Costruttore: salva i token e imposta l'indice iniziale a 0
Parser::Parser(const std::vector<std::string>& tokens)
	: _tokens(tokens), _index(0) {}


// peek() restituisce il token corrente SENZA avanzare l'indice.
// Se siamo alla fine della lista, restituisce una stringa vuota.
std::string Parser::peek() {
	if (_index >= _tokens.size())      // Se abbiamo superato l'ultimo token
		return "";                     // Restituiamo una stringa vuota
	return _tokens[_index];            // Altrimenti restituiamo il token corrente
}


// get() restituisce il token corrente e AVANZA al successivo.
// Se non ci sono più token, lancia un errore.
std::string Parser::get() {
	if (_index >= _tokens.size())      // Controllo di sicurezza
		throw std::runtime_error("Fine inattesa dei token");
	return _tokens[_index++];          // Restituisce il token e incrementa l’indice
}


// parse() è il punto d'ingresso principale del parser.
// Qui viene creato il "blocco radice" che conterrà tutto il file.
Block Parser::parse() {
	Block root;                        // Crea un blocco principale che conterrà tutto
	root.type = "root";                // Tipo "root" per distinguere il blocco base

	// Scorri tutti i token
	while (_index < _tokens.size()) {
		// Se il token è "server", significa che inizia un blocco server { ... }
		if (peek() == "server")
			root.sub_blocks.push_back(parseBlock()); // Analizza tutto il blocco e aggiungilo come sotto-blocco
		else
			// Altrimenti è una direttiva globale (es. error_page, max_request_body)
			root.directives.push_back(parseDirective());
	}
	return root;                       // Restituisce il blocco completo (l’albero di configurazione)
}


// parseBlock() viene chiamato quando incontriamo un blocco tipo "server" o "location"
Block Parser::parseBlock() {
	Block block;                       // Crea un nuovo blocco
	block.type = get();                // Legge il tipo (es. "server" o "location")

	// Se è un blocco location, il prossimo token è il percorso (es. "/ciao")
	if (block.type == "location")
		block.param = get();           // Salva "/ciao" in block.param

	// Controlla che il prossimo token sia "{" altrimenti errore di sintassi
	if (get() != "{")
		throw std::runtime_error("Errore di sintassi: manca '{' dopo " + block.type);

	// Finché non troviamo "}", leggiamo le direttive o sottoblocchi
	while (peek() != "}") {
		if (peek().empty())            // Se i token finiscono prima della '}', è un errore
			throw std::runtime_error("Errore: manca '}' di chiusura per " + block.type);

		if (peek() == "location")      // Se troviamo "location", è un sottoblocco
			block.sub_blocks.push_back(parseBlock());
		else
			// Altrimenti è una normale direttiva
			block.directives.push_back(parseDirective());
	}

	get();                             // Consuma il token '}' finale
	return block;                      // Restituisce il blocco completato
}


// parseDirective() viene chiamato quando troviamo una riga come:
// root ./www;  oppure  autoindex on;
Directive Parser::parseDirective() {
	Directive dir;                    // Crea una nuova direttiva
	dir.name = get();                 // Il primo token è il nome (es. "root")

	// Finché non troviamo ';', leggiamo tutti gli argomenti
	while (peek() != ";") {
		if (peek().empty())           // Se finisce il file prima del ';', errore
			throw std::runtime_error("Errore di sintassi: direttiva senza ';' dopo " + dir.name);
		dir.args.push_back(get());    // Aggiungiamo l’argomento (es. "./www")
	}

	get();                             // Consuma il ';' finale
	return dir;                        // Restituisce la direttiva completata
}
