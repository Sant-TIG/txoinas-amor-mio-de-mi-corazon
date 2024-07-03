#ifndef SERVER_HPP

# define SERVER_HPP

# include "Location.hpp"
# include "Utils.hpp"
# include "Colors.hpp"
# include <netinet/in.h>
# include <string>
# include <sstream>
# include <sys/socket.h>
# include <sys/select.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <map>

class Server {
	private:
		uint16_t						_port;
		std::string						_server_name;
		in_addr_t						_host;//uint32_t mas generica
		std::string						_root;
		std::string						_index;
		size_t							_cmbz;//client_max_body_size
		std::map<short, std::string>	_error_pages;
		std::vector<Location>			_locations;
		
		std::vector<std::string>		_directives;
	
	public:
		Server( void );
		Server( const Server &other );
		Server	&operator=( const Server &other );
		~Server( void );

		const uint16_t						&getPort( void ) const;
		const std::string					&getServerName( void ) const;
		const in_addr_t						&getHost( void ) const;
		const std::string					&getRoot( void ) const;
		const std::string					&getIndex( void ) const;
		const size_t						&getClientMaxBodySize( void ) const;
		const std::map<short, std::string>	&getErrorPages( void ) const;
		const std::vector<Location>			&getLocations( void ) const;
		void								setPort( const std::string &directive );
		void								setServerName( const std::string &server_name );
		void								setHost( const std::string &host );
		void								setRoot( const std::string &directive );
		void								setIndex( const std::string &index );
		void								setClientMaxBodySize( const std::string &directive );
		void								setErrorPage( size_t *i );
		void								setLocation( size_t *i);

		void								createServer( const std::string &conf );
		void								splitDirectives( const std::string &config );
		uint16_t							validatePort( const std::string &port );
		in_addr_t							validateIP( const std::string &ip );
		short								validateErrorCode( const std::string &str );
		void								validateLocationPath( size_t *i, Location &location );
		void								validateLocationRoot( Location &location, size_t *i );
		void								validateLocationMethods( Location &location, size_t *i );
		void								validateLocationCgiPath( Location &location, size_t *i );
		void								validateLocationCgiExt( Location &location, size_t *i );
		//void								validateLocationCgi( Location &location );
		//void								checkDuplicatedLocation( void );
		void								validateLocation( void );
		void								checkIndex( const std::string &index );
		class DirectiveError : public std::exception {
			private:
				std::string _msg;

			public:
				DirectiveError( const std::string &msg ) throw() : _msg("webserver: directive_error: " + msg) {}
				~DirectiveError( void ) throw() {}
				virtual const char	*what() const throw() {
					return _msg.c_str();
				}
		};
};

#endif