#include "Utils.hpp"

/*
	Comprueba el tipo de elemento que representa la ruta pasada como parametro
	La estructura stat almacena la informacion de un archivo.
*/
int	checkTypePath( const std::string &path ) {
	struct stat	info;
	std::cout << "Comprobando tipo de ruta: " << path << std::endl;
	if (stat(path.c_str(), &info) != 0) {
		std::cout << "No se pudo acceder a la ruta" << std::endl;
		return PATH_NOT_FOUND;
	} else if (S_ISREG(info.st_mode)) {
		std::cout << "Es un archivo" << std::endl;
		return IS_FILE;
	} else if (S_ISDIR(info.st_mode)) {
		std::cout << "Es un directorio" << std::endl;
		return IS_DIRECTORY;
	} else {
		return IS_OTHER;
	}
}

bool	checkSemiColon( const std::string &str ) {
	return str[str.size() - 1] == ';';
}

std::string	cutSemiColon( const std::string &str ) {
	return str.substr(0, str.size() - 1);
}

bool	strIsNumber( const std::string &str ) {
	for (size_t i = 0; i < str.size(); i++) {
		if (!std::isdigit(static_cast<unsigned char>(str[i]))) {
			return false;
		}
	}
	return true;
}

void	showVector( std::vector<std::string> v1, std::string msg) {
	for (size_t i = 0; i < v1.size(); i++) {
		std::cout << YELLOW << msg << i << RESET << std::endl;
		std::cout << v1[i] + "\n" << std::endl;
	}
}

const char	*numberIP(in_addr_t ip) {
	struct in_addr ip_struct;
	ip_struct.s_addr = ip;
	const char *ip_str = inet_ntoa(ip_struct);
	return ip_str;
}

void	showMap( std::map<short, std::string> m ) {
	std::map<short, std::string>::iterator it;
	for (it = m.begin(); it != m.end(); ++it) {
        std::cout << " - Codigo: " << it->first << " - Fichero: " << it->second << std::endl;
    }
}
