#ifndef LOCATION_HPP

# define LOCATION_HPP

/*Permiten controlar el acceso especifico a cada url*/

# include <string>
# include <vector>

class Location {
	private:
		std::string					_path;
		std::string					_root;
		std::string					_index;
		std::string					_return;
		std::string					_alias;
		std::string					_autoindex;
		std::vector<std::string>	_returns;
		std::vector<std::string>	_methods;
		std::vector<std::string>	_cgi_path;
		std::vector<std::string>	_cgi_ext;
		size_t						_cmbz; // client max body size

	public:
		Location();
		Location( const Location &other );
		Location &operator=( const Location &other );
		~Location();

		const std::string				&getPath( void ) const;
		const std::string				&getRoot( void ) const;
		const std::string				&getIndex( void ) const;
		const std::string				&getReturn( void ) const;
		const std::string				&getAlias( void ) const;
		const std::string				&getAutoIndex( void ) const;
		const std::vector<std::string>	&getReturns( void ) const;
		const std::vector<std::string>	&getMethods( void ) const;
		const std::vector<std::string>	&getCgiPath( void ) const;
		const std::vector<std::string>	&getCgiExt( void ) const;
		const size_t					&getClientMaxBodySize( void ) const;

		void							setPath( const std::string &path );
		void							setRoot( const std::string &root );
		void							setIndex( const std::string &index );
		void							setReturn( const std::string &directive );
		void							setAlias( const std::string &alias );
		void							setAutoIndex( const std::string &directive );
		//void							setReturns( const std::vector<std::string> &returns );
		void							setMethods( const std::vector<std::string> &methods );
		void							setCgiPath( const std::vector<std::string> &cgi_path );
		void							setCgiExt( const std::vector<std::string> &cgi_ext );
		void							setClientMaxBodySize( const std::string &directive );
		bool							searchMethod( const std::string &method );

		class LocationError : public std::exception {
			private:
				std::string _msg;

			public:
				LocationError( const std::string &msg ) throw() : _msg("webserver: location_error: " + msg) {}
				~LocationError( void ) throw() {}
				virtual const char	*what() const throw() {
					return _msg.c_str();
				}
		};
};

#endif