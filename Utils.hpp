#ifndef UTILS_HPP

# define UTILS_HPP

# include <string>
# include <vector>
# include <sys/stat.h>
# include <iostream>
# include "Colors.hpp"
# include <netinet/in.h>
# include <arpa/inet.h>
# include <map>

enum PathType {
	PATH_NOT_FOUND = -1,
	IS_FILE = 1,
	IS_DIRECTORY = 2,
	IS_OTHER = 3
};

int			checkTypePath( const std::string &path );
bool		checkSemiColon( const std::string &str );
std::string	cutSemiColon( const std::string &str );
bool		strIsNumber( const std::string &str );

void		showVector( std::vector<std::string> v1, std::string msg );
const char	*numberIP(in_addr_t ip);
void		showMap( std::map<short, std::string> m );

#endif