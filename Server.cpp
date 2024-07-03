#include "Server.hpp"

Server::Server( void ) : _port(0), _server_name(""), _host(0), _root(""), _index(""), _cmbz(0), _directives() {
	_error_pages[404] = "error_pages/404.html";
	//std::cout << GREEN << "Server default constructor called" << RESET << std::endl;
}

Server::Server( const Server &other ) {
	_port = other._port;
	_server_name = other._server_name;
	_host = other._host;
	_index = other._index;
	_cmbz = other._cmbz;
	_error_pages = other._error_pages;
	_directives = other._directives;
	_locations = other._locations;
	_root = other._root;
}

Server::~Server( void ) {
	//std::cout << RED << "Server destructor called" << RESET << std::endl;
}

Server	&Server::operator=( const Server &other ) {
	if (this != &other) {
		_port = other._port;
		_server_name = other._server_name;
		_host = other._host;
		_index = other._index;
		_cmbz = other._cmbz;
		_error_pages = other._error_pages;
		_directives = other._directives;
		_locations = other._locations;
		_root = other._root;
	}
	return (*this);
}

/* GETTERS */
const uint16_t	&Server::getPort( void ) const {
	return _port;
}

const std::string	&Server::getServerName( void ) const {
	return _server_name;
}

const in_addr_t	&Server::getHost( void ) const {
	return _host;
}

const std::string	&Server::getRoot( void ) const {
	return _root;
}

const std::string	&Server::getIndex( void ) const {
	return _index;
}

const size_t	&Server::getClientMaxBodySize( void ) const {
	return _cmbz;
}

const std::map<short, std::string>	&Server::getErrorPages( void ) const {
	return _error_pages;
}

const std::vector<Location>	&Server::getLocations( void ) const {
	return _locations;
}

/* SETTERS */

void	Server::setPort( const std::string &directive ) {
	if (_port != 0) {
		throw DirectiveError("the directive 'listen' is duplicated.");
	} else if (checkSemiColon(directive) == false) {
		throw DirectiveError("the directive 'listen' is not semicolon terminated.");
	}
	std::string port = cutSemiColon(directive);
	if (strIsNumber(port) == false) {
		throw DirectiveError("the specified port is not valid.\nPorts must be between 0 and 65535.");
	}
	_port = validatePort(port);
}

void	Server::setServerName( const std::string &server_name ) {
	if (_server_name != "") {
		throw DirectiveError("the directive 'server_name' is duplicated.");
	} else if (checkSemiColon(server_name) == false) {
		throw DirectiveError("the directive 'server_name' is not semicolon terminated.");
	}
	_server_name = cutSemiColon(server_name);
}

void	Server::setHost( const std::string &host ) {
	if (_host != 0) {
		throw DirectiveError("the directive 'host' is duplicated.");
	} else if (checkSemiColon(host) == false) {
		throw DirectiveError("the directive 'host' is not semicolon terminated.");
	}
	_host = validateIP(cutSemiColon(host));
}

void	Server::setRoot( const std::string &directive ) {
	if (_root != "") {
		throw DirectiveError("the directive 'root' is duplicated.");
	} else if (checkSemiColon(directive) == false) {
		throw DirectiveError("the directive 'root' is not semicolon terminated.");
	}
	std::string root = cutSemiColon(directive);
	if (checkTypePath(root.substr(1, root.size() -1)) == 2) {
		_root = root;
		return ;
	}
	char	cur_dir[4096];
	getcwd(cur_dir, 4096);
	//std::cout << "cur_dir: " << cur_dir << std::endl;
	if (checkTypePath(cur_dir + root) != 2) {
		std::cout << "cur_dir + root: " << cur_dir + root << std::endl;
		throw DirectiveError("the root path is not a directory.");
	}
	_root = cur_dir + root;
}

void	Server::setIndex( const std::string &index ) {
	if (_index != "") {
		throw DirectiveError("the directive 'index' is duplicated.");
	} else if (checkSemiColon(index) == false) {
		throw DirectiveError("the directive 'index' is not semicolon terminated.");
	}
	_index = cutSemiColon(index);
}

void Server::setClientMaxBodySize( const std::string &directive ) {
	size_t	size;
	
	size = 0;
	if (_cmbz != 0) {
		throw DirectiveError("the directive 'client_max_body_size' is duplicated.");
	} else if (checkSemiColon(directive) == false) {
		throw DirectiveError("the directive 'client_max_body_size' is not semicolon terminated.");
	}
	std::string cmbz = cutSemiColon(directive);
	if (strIsNumber(cmbz) == false) {
		throw DirectiveError("the specified body size is not valid.");
	}
	std::stringstream ss(cmbz);
	ss >> size;
	if (ss.fail() || !ss.eof()) {
		throw DirectiveError("the specified body size is not valid.");
	}
	_cmbz = size;
}

void	Server::setErrorPage( size_t *i ) {
	if (checkSemiColon(_directives[*i + 1]) == false) {
		throw DirectiveError("wrong syntax of 'error_page' directive.");
	}
	short error_code = validateErrorCode(_directives[*i]);
	std::map<short, std::string>::iterator it =_error_pages.find(error_code);
	if (it != _error_pages.end()) {
		_error_pages[error_code] = cutSemiColon(_directives[++(*i)]);
	} else {
		std::string	error_file = cutSemiColon(_directives[++(*i)]);
		if (checkTypePath(error_file) != 1) {
			throw DirectiveError("the path specified in 'error_page' directive is not a file.");
		}
		_error_pages.insert(std::make_pair(error_code, error_file));		
	}
}

/*QUE OCURRE SI FALTAN DIRECTIVAS: INDEX, CMBZ
SE LES ASIGNA EL DEL SERVER¿*/
void	Server::setLocation( size_t *i ) {
	Location new_location;

	validateLocationPath(i, new_location);
	while (*i < _directives.size()) {
		if (_directives[*i] == "root") {
			(*i)++;
			validateLocationRoot(new_location,i);
		} else if (_directives[*i] == "allow_methods") {
			validateLocationMethods(new_location, i);
		} else if (_directives[*i] == "autoindex") {
			new_location.setAutoIndex(_directives[++(*i)]);
		} else if (_directives[*i] == "index") {
			new_location.setIndex(_directives[++(*i)]);
		} else if (_directives[*i] == "return") {
			new_location.setReturn(_directives[++(*i)]);
		} else if (_directives[*i] == "alias") {
			new_location.setAlias(_directives[++(*i)]);
		} else if (_directives[*i] == "cgi_path") {
			validateLocationCgiPath(new_location, i);
		} else if (_directives[*i] == "cgi_ext") {
			validateLocationCgiExt(new_location, i);
		} else if (_directives[*i] == "client_max_body_size") {
			new_location.setClientMaxBodySize(_directives[++(*i)]);
		} else if (_directives[*i] == "}") {
			break;
		} else {
			throw DirectiveError("Wrong directive within directive 'location'.");
		}
		(*i)++;
	}
	std::cout << "Location: " << new_location.getPath() << std::endl;
	//if (_directives[*i - 1] != "}") {
//		throw DirectiveError("Wrong directive within directive 'location'.");
//	}
	_locations.push_back(new_location);
	std::cout << _locations.size() << std::endl;
}

/* OTHERS */

/*
*/
void	Server::createServer( const std::string &config ) {
	splitDirectives(config);
	//showVector(_directives, "Directiva ");
	for (size_t i = 0; i < _directives.size(); i++) {
		if (_directives[i] == "listen") {
			setPort(_directives[++i]);
		} else if (_directives[i] == "server_name") {
			setServerName(_directives[++i]);
		} else if (_directives[i] == "host") {
			setHost(_directives[++i]);
		} else if (_directives[i] == "root") {
			setRoot(_directives[++i]);
		} else if (_directives[i] == "index") {
			setIndex(_directives[++i]);
		} else if (_directives[i] == "client_max_body_size") {
			setClientMaxBodySize(_directives[++i]);
		} else if (_directives[i] == "error_page") {
			i++;
			setErrorPage(&i);
		} else if (_directives[i] == "location") {
			i++;
			setLocation(&i);
		}
	}
}

/*
   Como no todas las directivas son clave valor 1:1, no puedo guardarlos de forma
   sencilla en un diccionario, por lo que divido toda la configuracion de un servidor
   en sus palabras, y mas adelante ya se evaluaran si son directivas correctas */
void	Server::splitDirectives( const std::string &config ) {
	std::string spaces = " \t\n\r\v\f";
	std::size_t	start = 0, end;

	while (1) {
		end = config.find_first_of(spaces, start);
		if (end == std::string::npos) {
			break;
		}
		_directives.push_back(config.substr(start, end - start));
		start = config.find_first_not_of(spaces, end);
		if (start == std::string::npos) {
			break;
		}
	}
}

/*
	Valida 
*/
uint16_t	Server::validatePort( const std::string &port ) {
	std::stringstream ss(port);
	int temp;

	ss >> temp;
	if (ss.fail() || !ss.eof() || temp < 0 || temp > 65535) {
		throw DirectiveError("the specified port is not valid.\nPorts must be between 0 and 65535.");
	} else {
		return static_cast<uint16_t>(temp);
	}
}

in_addr_t	Server::validateIP( const std::string &ip ) {
	struct sockaddr_in socket;
	if (ip == "localhost") {
		return inet_addr(std::string("127.0.0.1").data());
	} else if (!inet_pton(AF_INET, ip.c_str(), &(socket.sin_addr))) {
		throw DirectiveError("the specified IP for host is not valid.");
	}
	return socket.sin_addr.s_addr;
}

short	Server::validateErrorCode( const std::string &str ) {
	short num;
	std::stringstream ss(str);

	// comprobamos que el primero este compuesto por digitos
	if (strIsNumber(str) == false) {
		throw DirectiveError("invalid 'error_page' code.");
	}
	ss >> num;
	// Comprobamos que sea un numero entero entre 400 y 600
	if (ss.fail() || !ss.eof() || num < 400 || num > 600) {
		throw DirectiveError("invalid 'error_page' code.");
	}
	return num;
}

void	Server::validateLocationPath( size_t *i, Location &location ) {
	if (_directives[*i] == "{" || _directives[*i] == "}") {
		throw DirectiveError("no se ha especificado ningun path en la directiva 'location'.");
	}
	location.setPath(_directives[*i]);
	if (_directives[++(*i)] != "{") {
		throw DirectiveError("hay mas de un argumento en la directiva location.");
	}
	(*i)++;
}

/*
Si no hay una directiva root en el bloque location, NGINX usará la directiva root definida en el bloque server.
*/
void	Server::validateLocationRoot( Location &location, size_t *i ) {
	if (location.getRoot() != "") {
		throw DirectiveError("the directive 'root' is duplicated within directive 'location'");
	} else if (checkSemiColon(_directives[*i]) == false) {
		throw DirectiveError("the directive 'root' is not semicolon terminated.");
	}
	std::string	root = cutSemiColon(_directives[(*i)]);
	if (checkTypePath(root.substr(1, root.size() -1)) == 2) {
		location.setRoot(root);
	} else if (checkTypePath(_root + root) == 2) {
		location.setRoot(root);
	} else {
		throw DirectiveError("the root path is not a directory.");
	}
}

void	Server::validateLocationMethods( Location &location, size_t *i ) {
	std::vector<std::string>	methods;

	while (++(*i) < _directives.size()) {
		if (!checkSemiColon(_directives[*i])) {
			methods.push_back(_directives[*i]);
		} else {
			methods.push_back(cutSemiColon(_directives[*i]));
			break ;
		}
	}
	location.setMethods(methods);
}

void	Server::validateLocationCgiPath( Location &location, size_t *i ) {
	std::vector<std::string>	paths;

	while (++(*i) < _directives.size()) {
		if (!checkSemiColon(_directives[*i])) {
			paths.push_back(_directives[*i]);
		} else {
			paths.push_back(cutSemiColon(_directives[*i]));
			break ;
		}
		if (_directives[*i].find("/python") == std::string::npos && _directives[*i].find("/bash") == std::string::npos)
			throw DirectiveError("cgi_path is invalid");
	}
	location.setCgiPath(paths);
}

void	Server::validateLocationCgiExt( Location &location, size_t *i ) {
	std::vector<std::string>	extensions;

	while (++(*i) < _directives.size()) {
		if (!checkSemiColon(_directives[*i])) {
			extensions.push_back(_directives[*i]);
		} else {
			extensions.push_back(cutSemiColon(_directives[*i]));
			break ;
		}
	}
	location.setCgiExt(extensions);
}

void	Server::checkIndex( const std::string &index ) {
	if (checkTypePath(index) != 1 || access(index.c_str(), R_OK) != 0) {
		throw (DirectiveError("Index mal especificado en la location."));
	}
}

void	Server::validateLocation( void ) {
	if (_locations.size() > 1) {
		for (std::vector<Location>::iterator it = _locations.begin(); it != _locations.end(); it++) {
			if (!it->getIndex().empty()) {
				//std::cout << "Index: " << it->getPath() << "/" << it->getIndex() << std::endl;
				checkIndex(std::string(".") +  it->getPath() + std::string("/") + it->getIndex());
			}
			for (std::vector<Location>::iterator it2 = it + 1; it2 != _locations.end(); it2++) {
				if (it->getPath() == it2->getPath()) {
					throw DirectiveError("El path de la location esta repetida.");
				}
			}
		}
	}
}
