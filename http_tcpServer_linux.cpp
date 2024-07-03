#include "http_tcpServer_linux.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <dirent.h>
#include <libgen.h>

namespace http
{

    const int BUFFER_SIZE = 30000;
    const int MAX_CLIENTS = 3;

    void log(const std::string &message)
    {
        std::cout << message << std::endl;
    }

    void exitWithError(const std::string &errorMessage)
    {
        log("ERROR: " + errorMessage);
        exit(1);
    }

    /*   TcpServer::TcpServer(in_addr_t	 ip_address, uint16_t port) : m_ip_address(ip_address), m_port(port), m_socket(), m_new_socket(),
                                                                m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress)),
                                                                m_serverMessage(buildResponse())
       {
           m_socketAddress.sin_family = AF_INET;
           m_socketAddress.sin_port = htons(m_port);
           m_socketAddress.sin_addr.s_addr = m_ip_address;

           if (startServer() != 0)
           {
               std::ostringstream ss;
               ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
               log(ss.str());
           }
       }
       */

    bool TcpServer::_endserver = false;
    TcpServer::TcpServer(const WebServer &webServer) : webServer(webServer)
    {
        signal(SIGINT, TcpServer::handle_sigint);
        const std::vector<Server> &servers = webServer.getServers();
        requestIsComplete = false;
        for (std::vector<Server>::const_iterator it = servers.begin(); it != servers.end(); ++it)
        {
            int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket < 0)
            {
                exitWithError("Socket creation failed");
            }

            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = it->getHost();
            serverAddr.sin_port = htons(it->getPort());

            if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
            {
                exitWithError("Socket bind failed");
            }

            if (listen(serverSocket, 20) < 0)
            {
                exitWithError("Socket listen failed");
            }

            serverSockets.push_back(serverSocket);

            std::ostringstream ss;
            ss << "\n*** Listening on ADDRESS: " << inet_ntoa(serverAddr.sin_addr) << " PORT: " << ntohs(serverAddr.sin_port) << " ***\n\n";
            log(ss.str());
        }
    }

    TcpServer::~TcpServer()
    {
        for (std::vector<int>::iterator it = serverSockets.begin(); it != serverSockets.end(); ++it)
        {
            close(*it);
        }
    }

    void TcpServer::handle_sigint(int sig)
    {
        _endserver = true;
        std::cout << "\nCaught signal " << sig << ", exiting..." << std::endl;
    }

    void TcpServer::startListen()
{
    fd_set readfds;
    fd_set writefds;
    int clientSockets[MAX_CLIENTS] = {0};
    int max_sd;

    while (!_endserver)
    {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        max_sd = 0;

        // Add server sockets to readfds
        for (std::vector<int>::iterator it = serverSockets.begin(); it != serverSockets.end(); ++it)
        {
            FD_SET(*it, &readfds);
            if (*it > max_sd)
            {
                max_sd = *it;
            }
        }

        // Add client sockets to readfds and writefds
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = clientSockets[i];
            if (sd > 0)
            {
                FD_SET(sd, &readfds);
                if (!_requests[sd].empty())
                {
                    FD_SET(sd, &writefds);
                }
                if (sd > max_sd)
                {
                    max_sd = sd;
                }
            }
        }

        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int activity = select(max_sd + 1, &readfds, &writefds, NULL, &tv);
        if ((activity < 0) && (errno != EINTR))
        {
            log("Select error");
            continue;
        }

        // Handle new connections
        for (std::vector<int>::iterator it = serverSockets.begin(); it != serverSockets.end(); ++it)
        {
            if (FD_ISSET(*it, &readfds))
            {
                acceptConnection(*it);
                if (m_new_socket >= 0)
                {
                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (clientSockets[i] == 0)
                        {
                            clientSockets[i] = m_new_socket;
                            break;
                        }
                    }
                }
            }
        }

        // Handle read and write operations
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = clientSockets[i];

            if (FD_ISSET(sd, &readfds))
            {
                std::string request;
                if (_requests[sd].empty())
                    checkRequest(sd, request);
            }

            if (FD_ISSET(sd, &writefds))
            {
                std::string request = _requests[sd];
                if (sendResponse(sd, request))
                {
                    _requests.erase(sd);
                    close(sd);
                    clientSockets[i] = 0;
                }
            }
        }
    }

    // Close all client sockets with 503 response on shutdown
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        int sd = clientSockets[i];
        if (sd > 0)
        {
            std::string response = "HTTP/1.1 503 Service Unavailable\r\nContent-Type: text/html\r\n\r\n"
                                   "<!DOCTYPE html><html><head><title>503 Service Unavailable</title></head><body>"
                                   "<h1>503 Service Unavailable</h1>"
                                   "<p>The server is shutting down.</p>"
                                   "</body></html>";
            send(sd, response.c_str(), response.length(), 0);
            close(sd);
        }
    }

    // Close all server sockets
    for (std::vector<int>::iterator it = serverSockets.begin(); it != serverSockets.end(); ++it)
    {
        close(*it);
    }

    exit(0);
}
    void TcpServer::acceptConnection(int serverSocket)
    {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        m_new_socket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        if (m_new_socket < 0)
        {
            exitWithError("Socket accept failed");
        }

        if (fcntl(m_new_socket, F_SETFL, O_NONBLOCK) < 0)
        {
            exitWithError("Failed to set client socket as non-blocking");
        }

        log("New connection accepted");
    }

    bool TcpServer::handleClientRead(int clientSocket)
    {
        char buffer[BUFFER_SIZE];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead < 0)
        {
            log("Read error");
            return false;
        }
        else if (bytesRead == 0)
        {
            log("Client disconnected");
            return false;
        }
        else
        {
            // Procesar la solicitud del cliente
            std::string request(buffer, bytesRead);
            log("Request received: " + request);
            checkRequest(clientSocket, request);
            return true;
        }
    }

    void TcpServer::handleClientWrite(int clientSocket)
    {
        // Manejar las respuestas del cliente si es necesario
        // Ejemplo simple de envío de una respuesta estática
        std::string response = buildResponse();
        send(clientSocket, response.c_str(), response.length(), 0);
    }

    /*void TcpServer::acceptConnection(int serverSocket)
    {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        m_new_socket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        if (m_new_socket < 0)
        {
            exitWithError("Socket accept failed");
        }

        if (fcntl(m_new_socket, F_SETFL, O_NONBLOCK) < 0)
        {
            exitWithError("Failed to set client socket as non-blocking");
        }

        log("New connection accepted");
    }*/

    std::string TcpServer::buildResponse()
    {
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from your Server :) </p></body></html>";
        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n"
           << htmlFile;

        return ss.str();
    }

    std::string TcpServer::readRequest(int clientsocket)
    {
        char buffer[1024];
        ssize_t bytesReceived;
        std::string requestContent;

        while ((bytesReceived = recv(clientsocket, buffer, sizeof(buffer), 0)) > 0)
        {
            if (bytesReceived < 0)
                exitWithError("Failed to read bytes from client socket connection");
            requestContent.append(buffer, bytesReceived);
        }
        return (requestContent);
    }

    std::string TcpServer::extractCGIScript(const std::string &url, Location location)
    {
        size_t cgiPos = url.find(location.getPath());
        if (cgiPos == std::string::npos)
        {
            return "";
        }
        size_t queryPos = url.find('?', cgiPos);
        std::string path_info;
        if (queryPos == std::string::npos)
        {
            path_info = url.substr(cgiPos);
        }
        else
        {
            path_info = url.substr(cgiPos, queryPos - cgiPos);
        }
        std::vector<std::string> ext = location.getCgiExt();
        //std::vector<std::string>::iterator it = ext.begin();
        for (std::vector<std::string>::iterator it = ext.begin(); it != ext.end(); ++it)
        {
            if (path_info.find(*it) != std::string::npos)
            {
                return path_info;
            }
        }
        return "";
    }

    /* std::string TcpServer::extractCGIScript(const std::string &url)
    {
         // Busca la parte del URL que indica el script CGI
         size_t cgiPos = url.find("/cgi/");
         if (cgiPos == std::string::npos)
         {
             return "";
         }
         size_t queryPos = url.find('?', cgiPos);
         std::string path_info;
         if (queryPos == std::string::npos)
         {
             path_info = "";
         }
         else
         {
             path_info = url.substr(cgiPos, queryPos - cgiPos);
         }
         return path_info;
     }*/

    Server TcpServer::getServerUsing(std::string request)
    {
        std::string sv;
        std::vector<Server> servInUse = webServer.getServers();
        std::cout << "*****************1***************" << std::endl;
        // std::cout << request << std::endl;
        std::string::size_type pos = request.find("Host: ");
        std::cout << "*****************host pos:" << pos << "***************" << std::endl;
        std::cout << request.substr(0, 100) << std::endl;
        if (pos != std::string::npos)
        {
            sv = request.substr(pos + 6, request.find("\n", pos) - pos - 7);
        }
        else
        {
            std::cout << "*****************2***************" << std::endl;
            throw std::runtime_error("No host found in the request");
        }
        std::cout << "*****************3***************" << std::endl;
        std::cout << "Server name:" << sv << "len:" << sv.length() << "*" << std::endl;
        for (std::vector<Server>::iterator it = servInUse.begin(); it != servInUse.end(); ++it)
        {
            // std::cout << "Server name: " << it->getServerName() << std::endl;
            std::ostringstream ss;
            ss << it->getPort();
            std::string test = std::string(numberIP(it->getHost())) + ":" + ss.str();
            std::cout << "Server name bueno:" << test << "*" << "len=" << test.length() << std::endl;
            std::cout << "compare" << test.compare(sv) << std::endl;
            if (test.compare(sv) == 0)
            {
                return *it;
            }
        }

        throw std::runtime_error("No server found with the given host name");
    }

    Location TcpServer::getLocation(std::string url, Server server)
    {
        std::vector<Location> locations = server.getLocations();
        Location bestMatch;
        std::string bestPath;

        std::vector<Location>::iterator it;
        for (it = locations.begin(); it != locations.end(); ++it)
        {
            std::string path = it->getPath();
            std::cout << "Location: " << path << std::endl;

            if (url.find(path) == 0 && path.length() > bestPath.length())
            {
                bestPath = path;
                bestMatch = *it;
            }
        }
        std::cout << "bestPath: " << bestPath << std::endl;
        if (bestPath.empty() || (url.compare(0, bestPath.length(), bestPath) != 0 && "favicon.ico" != url))
        {
            Location error;
            
            return error;
        }

        return bestMatch;
    }
    bool TcpServer::findMethod(std::string method, Location location)
    {
        std::vector<std::string> methods = location.getMethods();
        std::vector<std::string>::iterator it;

        for (it = methods.begin(); it != methods.end(); ++it)
        {
            if (*it == method)
            {
                return true;
            }
        }
        return false;
    }

    bool TcpServer::checkBodysize(std::string requestContent, Location location)
    {
        size_t pos = requestContent.find("\r\n\r\n");
        std::string body = requestContent.substr(pos + 4);
        if (body.length() > location.getClientMaxBodySize())
        {
            return false;
        }
        return true;
    }

    int TcpServer::strToNumber( const std::string &str ) {
        std::stringstream ss(str);
        int num;

        ss >> num;
        if (ss.fail() || !ss.eof()) {
            throw Server::DirectiveError("Error al convertir la cadena a numero.");
        }
        return num;

    }

    bool TcpServer::checkClientComplete(std::string requestContent)
    {
        std::cout << "EEEEEENNNNNNTTTTRRRRAAAA" << std::endl;
        size_t contentLengthPos = requestContent.find("Content-Length: ");
        if (contentLengthPos != std::string::npos)
        {
            size_t start = contentLengthPos + strlen("Content-Length: ");
            size_t end = requestContent.find("\r\n", start);
            int contentLength = strToNumber(requestContent.substr(start, end - start));//std::stoi(requestContent.substr(start, end - start));

            // Paso 3: Buscar el inicio del cuerpo del mensaje
            size_t bodyStartPos = requestContent.find("\r\n\r\n");
            if (bodyStartPos != std::string::npos)
            {
                // Asegurarse de que bodyStartPos apunte al inicio del cuerpo, no al inicio de los separadores
                bodyStartPos += 4;

                // Paso 4: Calcular la longitud del cuerpo del mensaje
                size_t bodyLength = requestContent.length() - bodyStartPos;

                // Paso 5: Comparar la longitud del cuerpo con el valor de Content-Length
                if (bodyLength == (size_t)contentLength)
                {
                    
                    std::cout << "El cuerpo del mensaje está completo." << std::endl;
                    return true;
                }
                else
                {
                    // El cuerpo del mensaje no está completo
                    std::cout << "Error: El cuerpo del mensaje no está completo." << std::endl;
                    return false;
                    // Aquí puedes manejar el error, por ejemplo, enviando una respuesta de error al cliente
                }
            }
            else
            {
                std::cout << "Error: No se encontró el inicio del cuerpo del mensaje." << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "Error: No se encontró el encabezado Content-Length." << std::endl;
            return false;
        }
    }
    void TcpServer::checkRequest(int clientSocket, std::string check)
    {
        std::string requestContent = check;
        std::ostringstream ss;
        int bytesReceived;
        char buffer[BUFFER_SIZE] = {0};
        std::cout << "Reading from socket" << std::endl;

        bool ischunked = false;
        while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
        {
            std::cout << "Bytes received: " << bytesReceived << std::endl;
            if (bytesReceived < 0)
            {
                exitWithError("Failed to read bytes from client socket connection");
            }

            requestContent.append(buffer, bytesReceived);

            if (requestContent.find("Transfer-Encoding: chunked") != std::string::npos)
            {
                ischunked = true;
            }
            if (ischunked)
            {
                std::string::size_type pos = 0;
                std::cout << "requestContent: " << requestContent << "$$$$$$" << std::endl;
                pos = requestContent.find("\r\n\r\n", pos);
                if (pos != std::string::npos)
                {
                    break;
                }
                std::string body = requestContent.substr(pos + 4);
                std::cout << "pos: " << pos << "*" << body << "////////" << std::endl;
                // while ((pos = requestContent.find("\r\n", pos)) != std::string::npos)
                //{

                //}
            }

            if (bytesReceived < BUFFER_SIZE)
            {
                break;
            }
        }
        if (requestIsComplete)
        {
            requestIsComplete = false;
            return;
        }
        std::cout << "Request content: " << requestContent << std::endl;
        // hacer funcion para saber que server es
        Server server = getServerUsing(requestContent);
        std::cout << "root =" << server.getRoot() << std::endl;
        // std::cout << "Client request: " << requestContent << std::endl;
        ss << requestContent;
        std::string contenido = ss.str();

        // Parse method and URL from the HTTP request
        std::istringstream iss(contenido);
        std::string metodo, url;
        iss >> metodo >> url;

        // Extract CGI script path_info from URL

        // Handle different types of HTTP requests
        std::cout << "Searching for: " << url << " with method: " << metodo << std::endl;
        // funcion para saber el location que toca
        Location location = getLocation(url, server);
        std::cout << "Location conseguido: " << location.getPath() << std::endl;
        if (location.getPath().empty() && url.compare("/") != 0)
        {
            generateErrorPage(clientSocket, 404);
            return;
        }
        std::string path_info = extractCGIScript(url, location);
        std::cout << "CGI script requested: " << path_info << std::endl;
        if (checkBodysize(requestContent, location) == false /*&& checkClientComplete(requestContent)*/)
        {
            std::cout << "Error 413" << std::endl;
            std::stringstream response;
            response << "HTTP/1.1 " << 413 << " Payload Too Large\n"
                     << "Content-Type: text/html\n"
                     << "Content-Length: " << 100 << "\n\n";
            std::string r = response.str();
            _requests[clientSocket] = r;

            // generateErrorPage(clientSocket, 413);
            return;
        }

        std::cout << "autoindex =? " << location.getAutoIndex() << std::endl;
        if ((metodo == "GET" && findMethod("GET", location)) || (metodo == "GET" && url == "/") || (metodo == "GET" && url == "/favicon.ico"))
        {
            if (location.getAutoIndex().compare("on") == 0)
            {
                autoIndex(clientSocket, url, location);
            }
            else if (url == "/" || url == server.getRoot())
            {
                handleRoot(clientSocket, server, url);
            }
            else if (url == "/favicon.ico")
            {
                handleFavicon(clientSocket);
            }
            else if (url == "/upload")
            {
                showUpload(clientSocket);
            }
            else if (!path_info.empty())
            {
                std::cout << "*********"
                          << "GET"
                          << "*********"
                          << std::endl;
                getCGIvariables(clientSocket, url, path_info);
            }
            else
            {
                std::cout << "tiene que entrar aqui" << std::endl;
                handleFormulario(clientSocket, url, location);
            }
        }

        else if (metodo == "POST" && findMethod("POST", location))
        {
            std::cout << "entra en post" << url << std::endl;
            if (url == "/upload")
            {
                std::cout << "Entra para subir archivo" << std::endl;
                handleUpload(clientSocket, requestContent, location);
            }
            // else if (url == "/cgi/formulario.py")
            //{
            //     getCGIvariablesPost(clientSocket, url, requestContent,  path_info);
            // }
            else if (!path_info.empty())
            {
                getCGIvariablesPost(clientSocket, url, requestContent, path_info);
            }
            else
            {
                std::cout << "ERROR POST." << std::endl;

                std::string r = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                _requests[clientSocket] = r;
                // sendResponse(clientSocket, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
            }
        }
        else if (metodo == "DELETE" && findMethod("POST", location))
        {

            handleDelete(clientSocket, url);
        }
        else
        { // Method not allowed error
            std::stringstream response;
            response << "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/html\r\n\r\n"
                     << "<!DOCTYPE html><html><head><title>405 Method Not Allowed</title></head><body>"
                     << "<h1>405 Method Not Allowed</h1>"
                     << "</body></html>";

            std::cout << "ERROR." << std::endl;
            std::string r = response.str();
            _requests[clientSocket] = r;
            // sendResponse(clientSocket, response.str());
        }
    }

    // comprobar mediante curl las peticiones  curl -X POST -F "field=@txoi.txt" http://localhost:8085/
    void TcpServer::getCGIvariables(int client_socket, std::string &requestContent, std::string path_info)
    {

        size_t queryStart = requestContent.find("?");
        if (queryStart == std::string::npos)
        {
            std::cout << "No query string found in the URL" << std::endl;
            return;
        }

        std::string queryString = requestContent.substr(queryStart + 1);
        std::cout << "QUERY_STRING: " << queryString << std::endl;

        setenv("QUERY_STRING", queryString.c_str(), 1);
        handle_cgi_request(client_socket, getenv("QUERY_STRING"), getenv("PATH"), path_info);
    }

    void TcpServer::getCGIvariablesPost(int client_socket, std::string &requestContent, std::string ss, std::string path_info)
    {

        std::string name;
        std::string email;

        (void)requestContent;
        std::string buffer = ss;
        std::size_t pos = buffer.rfind('\n');

        std::string lastLine = buffer.substr(pos + 1);
        buffer = lastLine;
        std::cout << "after header =" << buffer << std::endl;
        // name = buffer.substr(0);
        // email = buffer.substr(buffer.find("email=") + 6);
        // std::cout << "name: " << name << std::endl;
        // std::cout << "email: " << email << std::endl;
        // setenv("NAME", name.c_str(), 1);
        // setenv("EMAIL", email.c_str(), 1);
        /*   name = requestContent.substr(requestContent.find("name=") + 5, requestContent.find("&") - 24);
            email = requestContent.substr(requestContent.find("email=") + 6, requestContent.find("HTTP/") - 6);
            std::cout << "name: " << name << std::endl;
            std::cout << "email: " << email << std::endl;
           */
        setenv("QUERY_STRING", (buffer).c_str(), 1);

        handle_cgi_request(client_socket, getenv("QUERY_STRING"), getenv("PATH"), path_info);
    }

    unsigned long long int TcpServer::time_dif(void)
    {
        unsigned long long int ms;
        static unsigned long long int start = 0;
        static struct timeval time;

        gettimeofday(&time, NULL);
        ms = time.tv_sec * 1000;
        ms += time.tv_usec / 1000;
        if (!start)
            start = ms;
        return (ms - start);
    }

    int TcpServer::ft_usleep(unsigned long long int time, int fd, std::string *response)
    {
        unsigned long long int base;
        char buffer[4096];
        base = time_dif();
        std::cout << "fd " << fd << std::endl;
        while (time_dif() <= time + base)
        {
            std::cout << "bytesRead: " << 123 << std::endl;
            ssize_t bytesRead = read(fd, buffer, 1);
            std::cout << "bytesRead despues: " << bytesRead << std::endl;
            if (bytesRead > 0)
            {
                *response += std::string(buffer, bytesRead);
            }
            if (bytesRead == 0)
                return (0);
            if (bytesRead < 0)
                return (-1);
            usleep(500);
        }
        return (1);
    }

    void TcpServer::handle_cgi_request(int clientsocket, char *env, char *env2, std::string path_info)
    {
        pid_t pid;
        std::cout << "***" << std::endl;
        std::string read = "";
        std::cout << "***" << std::endl;
        std::string location = "." + path_info;
        std::cout << "path_info" << path_info << std::endl;
        std::cout << "location: " << location << std::endl;
        int check = 0;
        int fd[2];
        //  int fd2[2];

        if (pipe(fd) < 0)
        {
            perror("pipe");
            return;
        }

        pid = fork();
        if (pid < 0)
        {
            perror("fork");
            return;
        }

        if (pid == 0)
        {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            const char *argv[] = {location.c_str(), env, env2, NULL};
            execv(argv[0], (char *const *)argv);
            perror("execv");
            exit(EXIT_FAILURE);
        }

        else
        {
            close(fd[1]);
            int status;
            int time = 0;
            std::cout << "check: " << check << std::endl;
            unsigned long long int base;
            base = time_dif();
            while (waitpid(pid, &status, WNOHANG) == 0)
            {
                if (time_dif() >= 5000 + base)
                {
                    kill(pid, SIGKILL);
                    time = 1;
                    break;
                }
                else
                    time = 0;
                usleep(500);
            }
            if (time == 1)
                std::cout << "bucle infinito" << std::endl;
            if (time == 0)
                std::cout << "bucle fin" << std::endl;
            std::cout << "check: " << check << std::endl;
            check = ft_usleep(3000, fd[0], &read);
            std::cout << "hola" << std::endl;
            if (check == 0 && !read.empty())
            {
                // sleep(5);
                std::stringstream response;
                response << "HTTP/1.1 200 OK\r\n"
                         << read;
                std::string r = response.str();
                _requests[clientsocket] = r;
                // sendResponse(clientsocket, response.str());
            }
            else if (check == 0 && time == 0)
            {
                // sleep(5);
                log("Error reading from pipe");
                std::stringstream response;
                response << "HTTP/1.1 500 OK\r\n"
                         << "Content-Type: text/plain\r\n"
                         << "\r\n"
                         << "Internal Server Error";
                std::string r = response.str();
                _requests[clientsocket] = r;
                // sendResponse(clientsocket, response.str());
            }
            else if (time == 1)
            {
                // sleep(5);
                std::stringstream response;
                response << "HTTP/1.1 504 OK\r\n"
                         << "Content-Type: text/plain\r\n"
                         << "\r\n"
                         << "Gateway Time-out";
                std::string r = response.str();
                _requests[clientsocket] = r;
                // sendResponse(clientsocket, response.str());
            }
        }
    }

    void TcpServer::handleDelete(int clientSocket, const std::string &url)
    {
        // Extract the file path from the URL
        std::string filePath = "." + url;
        std::cout << "borra archivo: " << filePath << std::endl;
        // Check if the file exists
        struct stat buffer;
        if (stat(filePath.c_str(), &buffer) != 0)
        {
            // File does not exist
            generateErrorPage(clientSocket, 404);
            return;
        }

        // Attempt to delete the file
        if (remove(filePath.c_str()) != 0)
        {
            // Failed to delete the file
            generateErrorPage(clientSocket, 500);
            return;
        }

        // Successfully deleted the file
        std::stringstream response;
        response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                 << "<!DOCTYPE html><html><head><title>200 OK</title></head><body>"
                 << "<h1>File Deleted Successfully</h1>"
                 << "</body></html>";

        std::string r = response.str();
        _requests[clientSocket] = r;
    }

    std::string TcpServer::getFilesList()
    {
        std::stringstream ss;
        DIR *dir;
        struct dirent *ent;
        std::string check_dir = "upload/";
        if ((dir = opendir(check_dir.c_str())) != NULL)
        {
            ss << "<h2>Archivos disponibles:</h2><ul>";
            while ((ent = readdir(dir)) != NULL)
            {
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
                {
                    ss << "<li><a href=\"upload/" << ent->d_name << "\">" << ent->d_name << "</a></li>";
                }
            }
            ss << "</ul>";
            closedir(dir);
        }
        return ss.str();
    }

    std::string TcpServer::getFilesIndex(std::string url)
    {
        std::stringstream ss;
        DIR *dir;
        struct dirent *ent;
        std::string newUrl = "." + url;
        std::string check_dir = newUrl.c_str();
        std::cout << "**Incluir en referencia: " << check_dir << "***" << std::endl;
        if ((dir = opendir(check_dir.c_str())) != NULL)
        {
            // ss << "<h2>Archivos disponibles:</h2><ul>";
            ss << "<h2>" << url << "</h2><ul>";
            while ((ent = readdir(dir)) != NULL)
            {
                // Skip the current directory
                if (strcmp(ent->d_name, ".") == 0)
                    continue;

                // Special handling for the parent directory
                if (strcmp(ent->d_name, "..") == 0)
                {
                    // Create a link to the parent directory
                    if (url.substr(url.length() - 1) == "/")
                        ss << "<li><a href=\"" << url << "..\">..</a></li>";
                    else
                        ss << "<li><a href=\"" << url << "/..\">..</a></li>";
                }
                else
                {
                    // Create a link to the file or subdirectory
                    if (url == "/")
                        ss << "<li><a href=\"" << url << ent->d_name << "\">" << ent->d_name << "</a></li>";
                    else
                        ss << "<li><a href=\"" << url << "/" << ent->d_name << "\">" << ent->d_name << "</a></li>";
                }

                std::cout << "la referencia que me crea" << ss.str() << std::endl;
            }
            ss << "</ul>";
            closedir(dir);
        }
        return ss.str();
    }
    void TcpServer::generateErrorPage(int clientSocket, int errorCode)
    {
        std::stringstream response;
        response << "HTTP/1.1 " << errorCode << " Not Found\n"
                 << "Content-Type: text/html\n"
                 << "Content-Length: " << 100 << "\n\n";
        std::string r = response.str();
        _requests[clientSocket] = r;
        // sendResponse(clientSocket, response.str());
    }

    void TcpServer::autoIndex(int clientSocket, std::string url, Location location)
    {

        std::string newUrl = "." + url;
        std::cout << "url para Autoindex: " << newUrl << std::endl;
        std::stringstream response;
        DIR *dir;
        std::string check_dir = newUrl.c_str();
        if ((dir = opendir(check_dir.c_str())) != NULL)
        {

            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: text/html\r\n"
                     << "\r\n"
                     << "<!DOCTYPE html><html><head><title>Poner el nombre de la ruta</title></head><body>"
                     << getFilesIndex(url)
                     << "</body></html>";
            std::string responseData = response.str();
            _requests[clientSocket] = responseData;
            return;
            // send(clientSocket, responseData.c_str(), responseData.length(), 0);
        }
        else
            handleFormulario(clientSocket, url, location);
    }

    void TcpServer::showUpload(int clientSocket)
    {
        std::stringstream response;
        response << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: text/html\r\n"
                 << "\r\n"
                 << "<!DOCTYPE html><html><head><title>Archivos en el servidor</title></head><body>"
                 << getFilesList()
                 << "<button id='deleteButton'>Eliminar Archivo</button>"
                 << "<div id='response'></div>"
                 << "<script>"
                 << "document.addEventListener('DOMContentLoaded', function() {"
                 << "var deleteButton = document.getElementById('deleteButton');"
                 << "var responseContainer = document.getElementById('response');"
                 << "deleteButton.addEventListener('click', function() {"
                 << "var fileName = prompt('Por favor, ingresa el nombre del archivo que deseas eliminar:');"
                 << "if (fileName) {"
                 << "var xhr = new XMLHttpRequest();"
                 << "xhr.open('DELETE', '/upload/' + fileName, true);"
                 << "xhr.onreadystatechange = function() {"
                 << "if (xhr.readyState === XMLHttpRequest.DONE) {"
                 << "if (xhr.status === 200) {"
                 << "responseContainer.innerText = 'Archivo eliminado exitosamente.';"
                 << "} else {"
                 << "responseContainer.innerText = 'Error al eliminar el archivo.';"
                 << "}"
                 << "}"
                 << "};"
                 << "xhr.send();"
                 << "}"
                 << "});"
                 << "});"
                 << "</script>"
                 << "</body></html>";
        std::string responseData = response.str();
        _requests[clientSocket] = responseData;
        // send(clientSocket, responseData.c_str(), responseData.length(), 0);
    }

    void TcpServer::handleUpload(int clientSocket, std::string requestContent, Location location)
    {
        std::string save = requestContent;
        std::ostringstream ss;
        std::cout << "lee del socket" << std::endl;

        // Look for the end of the headers
        size_t headersEnd = requestContent.find("\r\n\r\n");
        if (headersEnd != std::string::npos)
        {
            // Extract the headers
            std::string headers = requestContent.substr(0, headersEnd);
            std::cout << "headers: " << headers << std::endl;
            // Look for the Content-Type header
            size_t contentTypePos = headers.find("Content-Type: ");
            if (contentTypePos != std::string::npos)
            {
                // Extract the boundary string
                // size_t boundaryPos = headers.find("boundary=", contentTypePos);
                // if (boundaryPos != std::string::npos)
                // {

                // std::string boundary = headers.substr( headers.find("boundary=") + 9, headers.find("\r\n", headers.find("boundary=")));
                std::string boundary = headers.substr(headers.find("boundary=") + 9, headers.find("\r\n", headers.find("boundary=")) - (headers.find("boundary=") + 9));
                std::cout << "boundary: " << boundary << "*****" << std::endl;
                // Look for the end of the request
                std::string endMarker = "\r\n--" + boundary + "--\r\n";
                std::cout << "endMarker: " << endMarker << std::endl;
                //  std::cout << "requestContent: " << requestContent << "****fin*****" <<  std::endl;
                // std::cout << "request" << requestContent << "**" << std::endl;
                // TODO cambiar condicion del if porque no se sube archivos. si no recuerdas porque mpregunta a Cristian
                if (requestContent.find(endMarker.c_str()) != std::string::npos)
                {
                    std::string prueba = requestContent.substr(requestContent.find("\r\n--" + boundary));
                    // std::cout << "prueba: " << prueba << "**" << std::endl;
                    handleMultipartRequest(clientSocket, prueba, location);
                    return;
                }
                std::cout << "no encuentra el final" << std::endl;
                // }
            }
        }
        std::cout << "no encuentra final de los headers" << std::endl;
        // std::cout << "save: " << save << std::endl;
        checkRequest(clientSocket, save);
    }
    // este funciona con txt
    void TcpServer::handleMultipartRequest(int clientSocket, const std::string &requestContent, Location location)
    {
        // Encontrar el límite multipartido
        std::cout << "entra aqui" << std::endl;
        std::string boundaryStart = "--";
        size_t boundaryPos = requestContent.find(boundaryStart);
        if (boundaryPos == std::string::npos)
        {
            exitWithError("Invalid request format: No boundary found");
            return;
        }
        boundaryPos += boundaryStart.length();

        // Encontrar el límite completo (incluyendo "--" al principio y "\r\n" al final)
        size_t endOfBoundaryPos = requestContent.find("\r\n", boundaryPos);
        if (endOfBoundaryPos == std::string::npos)
        {
            exitWithError("Invalid request format: Boundary end not found");
            return;
        }
        std::string boundary = "--" + requestContent.substr(boundaryPos, endOfBoundaryPos - boundaryPos);

        std::cout << "boundary: " << boundary << std::endl;
        std::string boundary2 = boundary + "\r\n\r\n";

        std::cout << "el boundary es :" << boundary2 << std::endl;
        // Encontrar la posición del inicio de los datos binarios de la imagen
        size_t dataStart = requestContent.find(boundary2.c_str());
        /*if (dataStart == std::string::npos) {
            exitWithError("Invalid request format: No image data found");
            return;
        }*/
        dataStart += (boundary + "\r\n\r\n").length();
        // Extraer los datos binarios de la imagen
        std::string imageData = requestContent.substr(dataStart);
        // std::cout << "imagen: " << imageData << std::endl;
        // std::cout << "imagen: " << imageData << std::endl;
        //  Eliminar el último límite del boundary
        size_t boundaryEndPos = imageData.rfind(boundary);
        if (boundaryEndPos != std::string::npos)
        {
            imageData.erase(boundaryEndPos);
        }

        // Buscar el nombre del archivo a subir
        std::string filenamePrefix = "filename=\"";
        size_t filenameStart = requestContent.find(filenamePrefix);
        if (filenameStart == std::string::npos)
        {
            exitWithError("Invalid request format: Filename not found");
            return;
        }
        filenameStart += filenamePrefix.length();

        size_t filenameEnd = requestContent.find("\"", filenameStart);
        if (filenameEnd == std::string::npos)
        {
            exitWithError("Invalid request format: Filename end not found");
            return;
        }
        std::string filename = requestContent.substr(filenameStart, filenameEnd - filenameStart);

       
        // Guardar los datos binarios en un archivo
        std::cout << "guarda la imagen en el archivo" << "." + location.getPath() + "/" + filename << std::endl;
        std::ofstream imageFile(("." + location.getPath() + "/" + filename).c_str(), std::ios::out | std::ios::binary);
        if (!imageFile)
        {
            exitWithError("Failed to open image file for writing");
            return;
        }
        // prueba para borrar las primeras lineas
        size_t firstNewlinePos = imageData.find('\n');
        if (firstNewlinePos != std::string::npos)
        {
            // Buscamos la posición del segundo salto de línea
            size_t secondNewlinePos = imageData.find('\n', firstNewlinePos + 1);
            if (secondNewlinePos != std::string::npos)
            {
                size_t thirdNewlinePos = imageData.find('\n', secondNewlinePos + 1);
                if (thirdNewlinePos != std::string::npos)
                {
                    size_t fourthNewlinePos = imageData.find('\n', thirdNewlinePos + 1);
                    if (fourthNewlinePos != std::string::npos)
                    {
                        imageData = imageData.substr(fourthNewlinePos + 1);
                    }
                }
            }
        }
        std::cout << "imagen datos : " << imageData << "**" << std::endl;
        // acaba aqui
        // imageData = imageData.substr(1);
        // std::cout << "imagen datos : " << imageData << "**" << std::endl;
        imageFile.write(imageData.c_str(), imageData.length());
        // imageFile  << imageData.c_str();
        imageFile.close();

        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n Connection: close\r\n\r\nImage uploaded successfully.";
        // std::string r = response.str();
        _requests[clientSocket] = response;
        // sendResponse(clientSocket, response);
    }

    void TcpServer::handleRoot(int clientSocket, Server server, std::string url)
    {
        // Leer el contenido del archivo formulario.html
        std::string relocate;
        if (url.compare(server.getRoot()) != 0)
        {

            // Redirigir al root asignado
            relocate = "HTTP/1.1 301 Moved Permanently\r\n";
            relocate += "Location: " + server.getRoot() + "\r\n\r\n";
            _requests[clientSocket] = relocate;
            return;
        }
        std::ostringstream response;
        std::cout << "root:" << server.getRoot() << std::endl;
        std::string index = "." + server.getRoot() + server.getIndex();
        std::cout << "index: " << index << std::endl;
        std::ifstream file(index.c_str(), std::ios::binary);
        if (!file.good())
        {

            response << "HTTP/1.1 " << 404 << " Not Found\n"
                     << "Content-Type: text/html\n"
                     << "Content-Length: " << 100 << "\n\n";
            std::string r = response.str();
            _requests[clientSocket] = r;
            // sendResponse(clientSocket, response.str());
            return;
        }

        // Leer el contenido del archivo
        std::ostringstream fileContent;
        fileContent << file.rdbuf();
        std::string fileStr = fileContent.str();

        // Construir la respuesta HTTP con el contenido del archivo
        // std::ostringstream response;
        response << "HTTP/1.1 200 OK\n"
                 << "Content-Type: "
                 << "prueba"
                 << "\n"
                 << "Content-Length: " << fileStr.size() << "\n\n"
                 << fileStr;

        // Construir la respuesta HTTP con el contenido del archivo

        response << "HTTP/1.1 200 OK\n"
                 << "Content-Type: text/html\n"
                 << "Content-Length: " << fileStr.size() << "\n\n"
                 << fileStr;

        // Enviar la respuesta al cliente
        std::string r = response.str();
        _requests[clientSocket] = r;
        // sendResponse(clientSocket, response.str());
    }

    void TcpServer::handleFavicon(int clientSocket)
    {
        std::cout << "hola" << std::endl;
        std::ifstream file("favicon/favicon.ico");
        std::ostringstream fileContent;
        fileContent << file.rdbuf();
        std::string fileStr = fileContent.str();

        // Construir la respuesta HTTP con el contenido del archivo
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\n"
                 << "Content-Type: text/html\n"
                 << "Content-Length: " << fileStr.size() << "\n\n"
                 << fileStr;

        // Enviar la respuesta al cliente
        std::string r = response.str();
        _requests[clientSocket] = r;
        // sendResponse(clientSocket, response.str());
    }
    void TcpServer::handleFormulario(int clientSocket, std::string url, Location location)
    {
        std::string filePath = url;
        if (!location.getIndex().empty())
        {
            std::cout << "tiene index: " << location.getIndex() << std::endl;
            filePath = "." + location.getPath() + "/" + location.getIndex();
        }

        std::cout << "filePath: " << filePath << std::endl;
        if (filePath.compare(0, 1, "/") == 0)
        {
            filePath = "." + url;
        }
        std::cout << "filePath2: " << filePath << std::endl;
        std::ifstream file(filePath.c_str(), std::ios::binary);
        if (!file.good())
        {
            std::ostringstream response;
            response << "HTTP/1.1 " << 404 << " Not Found\n"
                     << "Content-Type: text/html\n"
                     << "Content-Length: " << 100 << "\n\n";
            std::string r = response.str();
            _requests[clientSocket] = r;
            // sendResponse(clientSocket, response.str());
            return;
        }

        // Leer el contenido del archivo
        std::ostringstream fileContent;
        fileContent << file.rdbuf();
        std::string fileStr = fileContent.str();

        // Construir la respuesta HTTP con el contenido del archivo
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\n"
                 << "Content-Type: "
                 << "prueba"
                 << "\n"
                 << "Content-Length: " << fileStr.size() << "\n\n"
                 << fileStr;

        // Enviar la respuesta al cliente
        std::string r = response.str();
        // std::cout << "response: " << r << "========" << std::endl;
        _requests[clientSocket] = r;
        // sendResponse(clientSocket, response.str());
    }

    bool TcpServer::sendResponse(int sd, const std::string &response)
{
    ssize_t totalSent = 0;
    ssize_t responseLength = response.length();

    while (totalSent < responseLength)
    {
        ssize_t sent = send(sd, response.c_str() + totalSent, responseLength - totalSent, 0);
        if (sent < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                continue;
            return false;
        }
        totalSent += sent;
    }

    return true;
}
}
