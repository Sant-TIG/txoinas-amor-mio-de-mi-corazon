#include "WebServer.hpp"

WebServer::WebServer( void ) : _servers() {

}

WebServer::~WebServer( void ) {

}

const std::vector<Server>	WebServer::getServers( void ) const {
	return _servers;
}

void	WebServer::setServers( const std::vector<Server> &servers ) {
	_servers = servers;
}

/*
	validar servidores
*/
void	WebServer::createServers( const std::vector<std::string> &conf ) {
	for (size_t i = 0; i < conf.size(); i++) {
		Server server;
		server.createServer(conf[i]);
		_servers.push_back(server);
	}
	validateServers();

}

/*
	No puede haber dos servidores con mismo host y puerto
*/
void	WebServer::validateServers( void ) {
	for (std::vector<Server>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
		for (std::vector<Server>::iterator it2 = it + 1; it2 != _servers.end(); ++it2) {
			if(it->getHost() == it2->getHost() && it->getPort() == it2->getPort()) {
				throw Server::DirectiveError("servidores repetidos.");
			}
		}
	}
	for (std::vector<Server>::iterator it = _servers.begin(); it != _servers.end(); ++it) {
		it->validateLocation();
	}
}