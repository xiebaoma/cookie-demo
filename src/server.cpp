#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#endif

#include <iostream>
#include <string>
#include <sstream> // 添加这行
#include <cstdlib>
#include <ctime>
#include "http_common.h"

class HttpServer
{
public:
    HttpServer(int port) : port_(port) {}

    bool init()
    {
#ifdef _WIN32
        // Initialize Windows Socket API (Winsock)
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cerr << "WSAStartup failed\n";
            return false;
        }
#endif

        // Create a socket for the server (TCP/IP socket)
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0)
        {
            std::cerr << "Socket creation failed\n";
            return false;
        }

        // Define server address and port
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;         // Address family (IPv4)
        server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any available network interface
        server_addr.sin_port = htons(port_);      // Convert port number to network byte order

        // Bind the socket to the server address and port
        if (bind(server_socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Bind failed\n";
            return false;
        }

        // Start listening for incoming connections (maximum of 5 connections in the queue)
        if (listen(server_socket_, 5) < 0)
        {
            std::cerr << "Listen failed\n";
            return false;
        }

        // Output the server's listening status
        std::cout << "Server listening on port " << port_ << std::endl;

        // Successfully initialized
        return true;
    }

    void run()
    {
        while (true)
        {
            sockaddr_in client_addr{};
            int client_len = sizeof(client_addr);

#ifdef _WIN32
            // Accept a new client connection on Windows
            SOCKET client_socket = accept(server_socket_, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket == INVALID_SOCKET)
#else
            // Accept a new client connection on Linux or other POSIX systems
            int client_socket = accept(server_socket_, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
            if (client_socket < 0)
#endif
            {
                // If the accept call fails, log the error and continue accepting other connections
                std::cerr << "Accept failed: " << strerror(errno) << "\n";
                continue;
            }

            // Handle the client connection
            handleClient(client_socket);

            // Close the client socket after handling the request
#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
        }
    }

private:
#ifdef _WIN32
    SOCKET server_socket_;
#else
    int server_socket_;
#endif
    int port_;

    // Parse HTTP request
    HttpMessage parseRequest(const std::string &request_str)
    {
        HttpMessage request;                 // Create an HttpMessage object to store the parsed request
        std::istringstream iss(request_str); // Create an input string stream from the request string
        std::string line;                    // Variable to hold each line of the request

        // Parse the request line (method, path, version)
        std::getline(iss, line);                                           // Read the first line (request line)
        std::istringstream request_line(line);                             // Create a string stream from the request line
        request_line >> request.method >> request.path >> request.version; // Extract method, path, and version

        // Parse the request headers
        while (std::getline(iss, line) && line != "\r" && line != "") // Continue until the end of headers (empty line or \r)
        {
            size_t colon_pos = line.find(':');  // Find the position of the colon separating the header key and value
            if (colon_pos != std::string::npos) // Ensure that a colon was found
            {
                std::string key = line.substr(0, colon_pos);    // Extract the key (before the colon)
                std::string value = line.substr(colon_pos + 2); // Extract the value (skip ": " after the colon)

                // If the value ends with a carriage return, remove it
                if (!value.empty() && value.back() == '\r')
                {
                    value.pop_back();
                }

                // Store the header key and value in the headers map
                request.headers[key] = value;
            }
        }

        return request; // Return the parsed HttpMessage object
    }

    // Generate HTTP response based on the request
    std::string generateResponse(const HttpMessage &request)
    {
        HttpMessage response;
        response.version = "HTTP/1.1";  // Set the HTTP version to 1.1
        response.status_code = "200";   // Set the status code to 200 (OK)
        response.status_message = "OK"; // Set the status message to "OK"

        // Create a new session cookie
        std::string session_id = "user_" + std::to_string(std::rand());                         // Generate a random session ID
        response.headers["Set-Cookie"] = "session_id=" + session_id + "; Max-Age=3600; Path=/"; // Set the cookie with expiration time of 1 hour
        response.headers["Content-Type"] = "text/plain";                                        // Set content type to plain text

        // Check if the request contains a cookie
        auto it = request.headers.find("Cookie");
        std::string response_body;
        if (it != request.headers.end()) // If the "Cookie" header is found
        {
            response_body = "Welcome back! Your cookie: " + it->second + "\n"; // Greet the user with their cookie
        }
        else
        {
            response_body = "Hello! This is your first visit.\n"; // Greet the user on their first visit
        }

        // Set the response body and the Content-Length header
        response.body = response_body;
        response.headers["Content-Length"] = std::to_string(response_body.length()); // Set the content length to match the response body size

        // Construct the full HTTP response as a string
        std::stringstream ss;
        ss << response.version << " " << response.status_code << " " << response.status_message << "\r\n"; // Add the status line
        for (const auto &header : response.headers)                                                        // Loop through all headers and add them
        {
            ss << header.first << ": " << header.second << "\r\n";
        }
        ss << "\r\n"
           << response.body; // Add a blank line between headers and body, then add the body

        return ss.str(); // Return the complete HTTP response as a string
    }

    void handleClient(int client_socket)
    {
        char buffer[4096] = {0};
        std::string request_str;

        int bytes_received;
        while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0)
        {
            buffer[bytes_received] = '\0';                         // Null-terminate the buffer
            request_str += buffer;                                 // Append the buffer's contents to request_str
            if (request_str.find("\r\n\r\n") != std::string::npos) // Check for end of HTTP headers
            {
                break;
            }
        }

        if (bytes_received > 0)
        {
            // Parse the request and generate a response
            HttpMessage request = parseRequest(request_str);  // Parse the request string into an HttpMessage object
            std::string response = generateResponse(request); // Generate a response based on the request

            // Send the response back to the client
            send(client_socket, response.c_str(), response.length(), 0);
        }

#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
    }
};

int main()
{
    std::srand(std::time(nullptr)); // Initialize the random seed using current time
    HttpServer server(8080);
    if (!server.init())
    {
        return 1;
    }
    server.run();
    return 0;
}