#include "Location.hpp"
#include "Server.hpp"
#include "Config.hpp"

Location::Location( void ) : _path(""), _root(""), _index(""), _return(""), _alias(""), _autoindex(""), _methods(), _cgi_path(), _cgi_ext(), _cmbz(0) {
}

Location::Location( const Location &other ) {
	_path = other._path;
	_root = other._root;
	_index = other._index;
	_return = other._return;
	_alias = other._alias;
	_autoindex = other._autoindex;
	_methods = other._methods;
	_cgi_path = other._cgi_path;
	_cgi_ext = other._cgi_ext;
	_cmbz = other._cmbz;
}

Location	&Location::operator=( const Location &other ) {
	if (this != &other) {
		_path = other._path;
		_root = other._root;
		_index = other._index;
		_return = other._return;
		_alias = other._alias;
		_autoindex = other._autoindex;
		_methods = other._methods;
		_cgi_path = other._cgi_path;
		_cgi_ext = other._cgi_ext;
		_cmbz = other._cmbz;
	}
	return (*this);
}

Location::~Location( void ) {

}

/* GETTERS */
const std::string	&Location::getPath( void ) const {
	return _path;
}

const std::string	&Location::getRoot( void ) const {
	return _root;
}

const std::string	&Location::getIndex( void ) const {
	return _index;
}

const std::string	&Location::getReturn( void ) const {
	return _return;
}

const std::string	&Location::getAlias( void ) const {
	return _alias;
}

const std::string	&Location::getAutoIndex( void ) const {
	return _autoindex;
}

const std::vector<std::string>	&Location::getMethods( void ) const {
	return _methods;
}

const std::vector<std::string>	&Location::getCgiPath( void ) const {
	return _cgi_path;
}

const std::vector<std::string>	&Location::getCgiExt( void ) const {
	return _cgi_ext;
}

const size_t	&Location::getClientMaxBodySize( void ) const {
	return _cmbz;
}


/* SETTERS */

void	Location::setPath( const std::string &path ) {
	if (checkTypePath(path.substr(1, path.size() -1)) == 2 ) {
		_path = path;
	} else {
		char	cur_dir[4096];
		getcwd(cur_dir, 4096);
		if (checkTypePath(cur_dir + path) != 2) {
			std::cout << "ruta de location invalida" << cur_dir + path << std::endl;
			throw Server::DirectiveError("ruta de location invalida");
		}
		_path = path;
	}
}

void	Location::setRoot( const std::string &root ) {
	_root = root;
}

void	Location::setIndex( const std::string &index ) {
	if (_index != "") {
		throw LocationError("the directive 'index' is duplicated.");
	} else if (checkSemiColon(index) == false) {
		throw LocationError("the directive 'index' is not semicolon terminated.");
	}
	_index = cutSemiColon(index);
}


/* CGI ADMITE return? */
void	Location::setReturn ( const std::string &value ) {
	if (_return != "") {
		throw LocationError("the directive 'return' is duplicated.");
	} else if (checkSemiColon(value) == false) {
		throw LocationError("the directive 'return' is not semicolon terminated.");
	}
	_return = cutSemiColon(value);
}

/*void	Location::setReturns( const std::vector<std::string> &returns ) {
	_returns = returns;
}*/

void	Location::setAlias( const std::string &alias ) {
	if (_alias != "") {
		throw LocationError("the directive 'alias' is duplicated.");
	} else if (checkSemiColon(alias) == false) {
		throw LocationError("the directive 'alias' is not semicolon terminated.");
	}
	_alias = cutSemiColon(alias);
}

/* CGI ADMITE AUTOINDEX? */
void	Location::setAutoIndex( const std::string &directive ) {
	if (_autoindex != "") {
		throw LocationError("the directive 'autoindex' is duplicated.");
	} else if (checkSemiColon(directive) == false) {
		throw LocationError("the directive 'autoindex' is not semicolon terminated.");
	}
	std::string autoindex = cutSemiColon(directive);
	if (autoindex != "on" && autoindex != "off") {
		throw LocationError("wrong syntax in directive 'autoindex'.");
	}
	_autoindex = autoindex;
}

/*vector que contiene los metodos introducidos en el conf*/
void	Location::setMethods( const std::vector<std::string> &methods ) {
	showVector(methods, "Metodo: ");
	if (methods.size() > 3) {
		throw LocationError("se han añadido mas metodos de los permitidos a la directiva 'allow methods'.");
	}
	for (size_t i = 0; i < methods.size(); i++) {
		if (methods[i] == "GET" && !searchMethod("GET")) {
			_methods.push_back(methods[i]);
		} else if (methods[i] == "POST" && !searchMethod("POST")) {
			_methods.push_back(methods[i]);
		} else if (methods[i] == "DELETE" && !searchMethod("DELETE")) {
			_methods.push_back(methods[i]);
		} else {
			throw LocationError("metodo repetido o no valido en la directiva 'allow_methods'.");
		}
	}
	_methods = methods;
}

void	Location::setCgiPath( const std::vector<std::string> &cgi_path ) {
	_cgi_path = cgi_path;
}

void	Location::setCgiExt( const std::vector<std::string> &cgi_ext ) {
	_cgi_ext = cgi_ext;
}

void	Location::setClientMaxBodySize( const std::string &directive ) {
	size_t	size;

	if (_cmbz != 0) {
		throw Server::DirectiveError("the directive 'client_max_body_size' from 'location' is duplicated.");
	} else if (checkSemiColon(directive) == false) {
		throw Server::DirectiveError("the directive 'client_max_body_size' from 'location' is not semicolon terminated.");
	}
	std::string cmbz = cutSemiColon(directive);
	if (strIsNumber(cmbz) == false) {
		throw Server::DirectiveError("the specified body size from 'location' is not valid.");
	}
	std::stringstream ss(cmbz);
	ss >> size;
	if (ss.fail() || !ss.eof()) {
		throw Server::DirectiveError("the specified body size from 'location' is not valid.");
	}
	_cmbz = size;
}

bool	Location::searchMethod( const std::string &method ) {
	for (size_t i = 0; i < _methods.size(); i++) {
		if (_methods[i] == method) {
			return true;
		}
	}
	return false;
}