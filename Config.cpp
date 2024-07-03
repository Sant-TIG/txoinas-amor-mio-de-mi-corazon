#include "Config.hpp"

/*

*/
Config::Config( void ) : _config_path(""), _config_content(""), _servers_conf() {
	std::cout << GREEN << "Config default constructor called" << RESET << std::endl;
}
/*

*/
Config::Config( const std::string &config_path ) : _config_path(config_path), _config_content(""), _servers_conf() {
	std::cout << GREEN << "Config parameterized constructor called" << RESET << std::endl;
}

/*

*/
Config::~Config( void ) {
	std::cout << RED << "Config destructor called" << RESET << std::endl;
}


/* GETTERS */

const std::string	&Config::getConfigPath( void ) const {
	return _config_path;
}

const std::string	&Config::getConfigContent( void ) const {
	return _config_content;
}

const std::vector<std::string>	&Config::getServersConf( void ) const {
	return _servers_conf;
}


/* SETTERS */

void	Config::setConfigPath( const std::string &path ) {
	_config_path = path;
}

void	Config::setConfigContent( const std::string &content ) {
	_config_content = content;
}

void	Config::setServersConf( const std::vector<std::string> &servers_conf ) {
	_servers_conf = servers_conf;
}


/* OTHERS */
/*
	Comprueba que el path proporcionado corresponda a un fichero.
	Lee el fichero y comprueba que no este vacio.
	*¿QUEREMOS ELIMINAR COMENTARIOS?*
	Divide el contenido del fichero por directivas "server".
*/
void	Config::takeServersConfig( void ) {
	if (checkTypePath(_config_path) != 1) {
		throw FileError("the path is not a file.");
	}
	_config_content = readConfig();
	if (_config_content.empty()) {
		throw FileError("the file is empty.");
	}
	splitServersConfig();
	//showVector(_servers_conf);
	//std::vector<std::string> vector1 = splitDirectives(_config_content);
	//showVector(vector1);
	//_servers_conf = splitDirectives(_config_content);
}

/*
	Lee el contenido del archivo de configuracion y lo devuelve todo en una string
	*¿ES NECESARIO COMPROBAR QUE SE PUEDE ACCEDER A EL O TAN SOLO CON COMPROBAR SI ESTA ABIERTO YA SE DA POR HECHO DE QUE ES ACCESIBLE?*
*/
std::string	Config::readConfig( void ) {
	std::ifstream		config_content(_config_path.c_str());
	std::stringstream	buffer;

	if (!config_content || !config_content.is_open()) {
		throw FileError("failed to open the file.");
	}
	buffer << config_content.rdbuf();
	config_content.close();
	return buffer.str();
}

/*
	Divide el contenido del fichero divido por directivas "server". Busca la directiva, consigue la posicion
	de las llaves y almacena el contenido en el vector de directivas.
*/
void	Config::splitServersConfig( void ) {
	size_t	start = 0;
	size_t	end;

	while (start < _config_content.length()) {
		if (!searchServer(&start)) {
			throw ConfigError("the directive is not supported. The 'server' directive is required.");
		}
		start = searchStartBracket(start + 6);
		end = searchEndBracket(start);
		if (end == 0) {
			throw ConfigError("missing closing brace '}' in 'server' directive.");
		}
		_servers_conf.push_back(_config_content.substr(start, end - start + 1));
		start = end + 1;
	}
}

/*
	Busca la directiva server. Esta es la unica directiva que puede estar fuera de llaves.
*/
bool	Config::searchServer( size_t *start ) {
	while (*start < _config_content.length() && isspace(_config_content[*start])) {
		(*start)++;
	}
	/*find devuelve la posicion de la primera ocurrencia de la subcadena. Si esta no coincide con
	la posicion en donde se ha encontrado el primer caracter no blanco significa que hay una directiva
	erronea.*/
	if (_config_content.find("server", *start) == *start) {
		return true;
	} else {
		return false;
	}
}

/*
	Comprueba que lo siguiente a la directiva "server" sea un inicio de llaves.
*/
size_t	Config::searchStartBracket( size_t start ) {
	while (start < _config_content.length() && isspace(_config_content[start])) {
		start++;
	}
	if (_config_content[start] == '{') {
		return start;
	} else {
		throw ConfigError("unexcepted character found outside of server scope.");
	}
}

/*
	Comprueba que las llaves de la directiva "server" esten cerradas. Comprueba tambien que todas las llaves
	de las subdirectivas esten cerradas.
*/
size_t	Config::searchEndBracket( size_t start ) {
	int		pair = 0;
	size_t	end = start + 1;

	while (end < _config_content.length()) {
		if (_config_content[end] == '{') {
			pair++;
		} else if (_config_content[end] == '}') {
			if (pair == 0) {
				return end;
			}
			pair--;
		}
		end++;
	}
	return 0;
}