#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <iostream>
#include <string>
#include <sstream>
#include "http_common.h"
#include "cookie_manager.h"

class HttpClient
{
public:
    HttpClient(const std::string &host, int port)
        : host_(host), port_(port) {}

    bool init()
    {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cerr << "WSAStartup failed\n";
            return false;
        }
#endif
        return true;
    }

    // Establish a connection to the server
    bool connect()
    {
        // Create a socket for the client
        client_socket_ = socket(AF_INET, SOCK_STREAM, 0); // Use IPv4 and TCP (stream) protocol

        // Check if the socket creation failed
#ifdef _WIN32
        if (client_socket_ == INVALID_SOCKET) // For Windows, check if socket creation returns INVALID_SOCKET
#else
        if (client_socket_ < 0) // For Linux/Unix systems, check if socket creation returns a negative value
#endif
        {
            std::cerr << "Socket creation failed\n"; // Print an error message if socket creation fails
            return false;                            // Return false to indicate failure
        }

        // Set up the server address structure
        sockaddr_in server_addr{};           // Initialize the server address structure to 0
        server_addr.sin_family = AF_INET;    // Set the address family to IPv4
        server_addr.sin_port = htons(port_); // Set the server port (convert to network byte order)

        // Convert the server's IP address from text to binary form
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) // Convert IP address (host_) to binary
        {
            std::cerr << "Invalid address\n"; // Print an error message if the address conversion fails
            return false;                     // Return false to indicate failure
        }

        // Establish the connection to the server
        if (::connect(client_socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) // Attempt to connect
        {
            std::cerr << "Connection failed\n"; // Print an error message if the connection fails
            return false;                       // Return false to indicate failure
        }

        // Return true if the connection was successful
        return true;
    }

    // Function to send an HTTP request
    bool sendRequest()
    {
        // Build the HTTP request
        HttpMessage request;
        request.method = "GET";                                        // Set the HTTP method to GET
        request.path = "/";                                            // Set the request path to "/"
        request.version = "HTTP/1.1";                                  // Set the HTTP version to 1.1
        request.headers["Host"] = host_ + ":" + std::to_string(port_); // Set the Host header with the host and port

        // If a stored cookie exists, add it to the request headers
        std::string stored_cookie = cookie_manager_.getCookie("session_id"); // Retrieve the session cookie
        if (!stored_cookie.empty())                                          // If the cookie is not empty
        {
            request.headers["Cookie"] = "session_id=" + stored_cookie; // Add the cookie to the request headers
        }

        // Build the request string
        std::stringstream ss;
        ss << request.method << " " << request.path << " " << request.version << "\r\n"; // Add the request line (method, path, version)
        for (const auto &header : request.headers)                                       // Iterate over each header and add it to the request string
        {
            ss << header.first << ": " << header.second << "\r\n";
        }
        ss << "\r\n"; // Add an empty line after the headers to separate them from the body

        std::string request_str = ss.str();                                         // Convert the stringstream to a string containing the full HTTP request
        if (send(client_socket_, request_str.c_str(), request_str.length(), 0) < 0) // Send the HTTP request over the socket
        {
            std::cerr << "Send failed\n"; // If sending the request fails, print an error message
            return false;                 // Return false to indicate failure
        }

        return receiveResponse(); // Call the receiveResponse function to handle the server's response
    }

    void cleanup()
    {
#ifdef _WIN32
        closesocket(client_socket_);
        WSACleanup();
#else
        close(client_socket_);
#endif
    }

private:
#ifdef _WIN32
    SOCKET client_socket_;
#else
    int client_socket_;
#endif
    std::string host_;
    int port_;
    CookieManager cookie_manager_;

    // Function to receive and parse the HTTP response from the server
    bool receiveResponse()
    {
        char buffer[4096];        // Buffer to store the received data
        std::string response_str; // String to accumulate the response
        int bytes_received;       // Variable to store the number of bytes received in each call to recv()

        // Loop to receive data from the server until a full response is received (identified by "\r\n\r\n")
        while ((bytes_received = recv(client_socket_, buffer, sizeof(buffer) - 1, 0)) > 0)
        {
            buffer[bytes_received] = '\0'; // Null-terminate the received data to safely add it to the string
            response_str += buffer;        // Append the received data to the response string

            // If the response contains the end of headers "\r\n\r\n", stop receiving more data
            if (response_str.find("\r\n\r\n") != std::string::npos)
            {
                break;
            }
        }

        // Check if there was an error while receiving data
        if (bytes_received < 0)
        {
            std::cerr << "Receive failed\n"; // Print error message if receiving data failed
            return false;                    // Return false to indicate failure
        }

        // Parse the "Set-Cookie" header in the response (if present)
        std::istringstream iss(response_str); // Create an input string stream to parse the response string
        std::string line;                     // Variable to store each line of the response

        // Skip the status line (first line of the response)
        std::getline(iss, line);
        std::cout << "Response: " << line << std::endl; // Print the status line (for debugging)

        // Parse the headers in the response
        while (std::getline(iss, line) && line != "\r" && line != "") // Read each line until an empty line (header end)
        {
            // Check if the header is "Set-Cookie", which indicates a cookie is being sent
            if (line.substr(0, 11) == "Set-Cookie:")
            {
                std::string cookie_str = line.substr(12);    // Extract the cookie value (after "Set-Cookie: ")
                size_t semicolon_pos = cookie_str.find(';'); // Find the position of the first semicolon
                if (semicolon_pos != std::string::npos)
                {
                    cookie_str = cookie_str.substr(0, semicolon_pos); // If a semicolon is found, trim the string to the cookie value before it
                }

                size_t equals_pos = cookie_str.find('='); // Find the position of the '=' in the cookie
                if (equals_pos != std::string::npos)
                {
                    std::string name = cookie_str.substr(0, equals_pos);                   // Extract the cookie name (before '=')
                    std::string value = cookie_str.substr(equals_pos + 1);                 // Extract the cookie value (after '=')
                    cookie_manager_.setCookie(name, value);                                // Store the cookie using a cookie manager (presumably a member function)
                    std::cout << "Cookie received: " << name << "=" << value << std::endl; // Output the received cookie for debugging
                }
            }
            std::cout << line << std::endl; // Print each header line for debugging
        }

        return true; // Return true to indicate successful response reception and processing
    }
};

int main()
{
    HttpClient client("127.0.0.1", 8080);

    if (!client.init())
    {
        return 1;
    }

    std::cout << "Connecting to server...\n";

    // Send three requests to demonstrate the persistence of the cookie
    for (int i = 0; i < 3; ++i)
    {
        if (!client.connect())
        {
            return 1;
        }

        std::cout << "\nSending request " << (i + 1) << "...\n";
        if (!client.sendRequest())
        {
            return 1;
        }

// Wait one second before sending the next request
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    client.cleanup();
    return 0;
}