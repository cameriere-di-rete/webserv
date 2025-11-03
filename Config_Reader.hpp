#ifndef CONFIGFILEREADER_HPP     // Include guard: evita di includere più volte lo stesso header
#define CONFIGFILEREADER_HPP

#include <string>                // Serve per usare std::string

// Questa classe serve a leggere il contenuto del file di configurazione.
// Non ha bisogno di essere istanziata, perché contiene solo metodi statici.
class ConfigFileReader {
public:
	// Metodo statico che prende come parametro il percorso del file di configurazione
	// e restituisce il contenuto completo del file come una stringa.
	static std::string readFile(const std::string& path);
};

#endif // Fine include guard
