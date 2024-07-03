#ifndef INCLUDED_HTTP_TCPSERVER_LINUX
#define INCLUDED_HTTP_TCPSERVER_LINUX

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include "Server.hpp"
#include "WebServer.hpp"


namespace http
{

    class TcpServer
    {
    public:
        TcpServer(in_addr_t	ip_address, uint16_t port);
        TcpServer(const WebServer& webServer);
        ~TcpServer();
        void startListen();

    private:
        int startServer();
        void closeServer();
        void acceptConnection(int m_socket);
        bool findMethod(std::string method, Location location);
        void checkRequest(int clientSocket, std::string request);
        std::string buildResponse();
        bool sendResponse(int sd, const std::string &response);
        void handleFormulario(int clientSocket, std::string url, Location location);
        void handleRoot(int clientSocket, Server Server, std::string url);
        void handleFavicon(int clientSocket);
        void handleUpload(int clientSocket, std::string requestContent, Location location);
        void handle_cgi_request(int clientsocket, char *env, char *,  std::string path_info);
        void handle_cgi_request_post(int clientSocket);
        Location getLocation(std::string url, Server server);
        void getCGIvariables(int client_socket, std::string &requestContent, std::string path_info);
        void getCGIvariablesPost(int client_socket, std::string &requestContent, std::string ss,  std::string path_info);
        void handleDelete(int clientSocket, const std::string& url);
        void handleMultipartRequest(int clientSocket, const std::string &requestContent, Location location);
        // void handleMultipartRequest(int clientSocket, const std::string& requestContent);
        void showUpload(int clientSocket);
        std::string getFilesList();
        std::string getFilesIndex(std::string url);
        void autoIndex(int clientSocket, std::string url, Location location);
        void generateErrorPage(int clientSocket, int errorCode);
        std::string readRequest(int clientsocket);
        unsigned long long int	time_dif(void);
        int	ft_usleep(unsigned long long int time, int fd, std::string *response);
        std::string extractCGIScript(const std::string& url, Location location);
        bool checkBodysize(std::string requestContent, Location location);
        bool handleClientRead(int clientSocket);
        void handleClientWrite(int clientSocket);
        Server getServerUsing(std::string request);
        bool checkClientComplete(std::string requestContent);
        bool requestIsComplete;
        int     strToNumber( const std::string &str );
        
        static void handle_sigint(int sig);
        WebServer webServer;
        std::vector<int> serverSockets;
         //static const int MAX_CLIENTS = 30;

        in_addr_t m_ip_address;
        std::map<int, std::string> _requests;
        uint16_t m_port;
        int m_socket;
        int m_new_socket;
        sockaddr_in m_socketAddress;
        socklen_t m_socketAddress_len;
        std::string m_serverMessage;
        static bool _endserver;
    };

} // namespace http

#endif