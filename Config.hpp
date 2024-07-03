#ifndef CONFIG_HPP

# define CONFIG_HPP

# include <iostream>
# include <string>
# include <vector>
# include <sys/stat.h>
# include <fstream>
# include <sstream>
# include <unistd.h>
# include "Colors.hpp"
# include "Utils.hpp"

class Config {
	private:
		std::string	_config_path;
		std::string	_config_content;
		std::vector<std::string>	_servers_conf;

	public:
		Config( void );
		Config( const std::string &config_path );
		~Config( void );

		const std::string				&getConfigPath( void ) const;
		const std::string				&getConfigContent( void ) const;
		const std::vector<std::string>	&getServersConf( void ) const;
		void							setConfigPath( const std::string &path );
		void							setConfigContent( const std::string &content );
		void							setServersConf( const std::vector<std::string> &servers_conf );

		void							takeServersConfig( void );
		std::string						readConfig( void );
		void							splitServersConfig( void );
		bool							searchServer( size_t *start );
		size_t							searchStartBracket( size_t start );
		size_t							searchEndBracket( size_t start );

	public:
		class FileError : public std::exception {
			private:
				std::string	_msg;
			
			public:
				FileError( const std::string &msg ) throw() : _msg("webserver: file_error: " + msg) {}
				~FileError( void ) throw() {}
				virtual const char	*what() const throw() {
					return _msg.c_str();
				}
		};

		class ConfigError : public std::exception {
			private:
				std::string _msg;

			public:
				ConfigError( const std::string &msg ) throw() : _msg("webserver: config_error: " + msg) {}
				~ConfigError( void ) throw() {}
				virtual const char	*what() const throw() {
					return _msg.c_str();
				}
		};
};

#endif