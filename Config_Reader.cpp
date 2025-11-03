#include "Config_Reader.hpp"  // Include il file header associato per avere la dichiarazione della classe
#include <fstream>               // Serve per usare std::ifstream (lettura da file)
#include <sstream>               // Serve per std::stringstream (per concatenare linee di testo)
#include <stdexcept>             // Serve per lanciare eccezioni con std::runtime_error

// Implementazione del metodo statico readFile.
// Questo metodo apre un file e ne legge tutto il contenuto in una stringa.
std::string ConfigFileReader::readFile(const std::string& path) {
	std::ifstream file(path.c_str());  // Crea un oggetto ifstream per aprire il file in sola lettura
	if (!file.is_open())               // Controlla se il file è stato aperto correttamente
		throw std::runtime_error("Impossibile aprire il file di configurazione: " + path);
		// Se non riesce, lancia un'eccezione con un messaggio d'errore

	std::stringstream buffer;          // Crea un buffer temporaneo per accumulare il testo del file
	buffer << file.rdbuf();            // Copia tutto il contenuto del file nel buffer
	return buffer.str();               // Restituisce il contenuto come stringa
}
