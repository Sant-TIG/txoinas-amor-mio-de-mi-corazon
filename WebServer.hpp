#ifndef WEBSERVER_HPP

# define WEBSERVER_HPP

# include <vector>
# include "Server.hpp"
# include <string>
# include <iostream>

class WebServer {
	private:

		std::vector<Server> _servers;

	public:
		WebServer( void );
		~WebServer( void );

		const std::vector<Server>	getServers( void ) const;
		void	setServers( const std::vector<Server> &servers );
		void	createServers( const std::vector<std::string> &conf );
		void	validateServers( void );
		
};

#endif