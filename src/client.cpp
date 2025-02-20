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
// Temporarily comment out until cookie_manager.h is properly set up
// #include "cookie_manager.h"

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

    bool connect()
    {
        client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
        if (client_socket_ == INVALID_SOCKET)
#else
        if (client_socket_ < 0)
#endif
        {
            std::cerr << "Socket creation failed\n";
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);

        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0)
        {
            std::cerr << "Invalid address\n";
            return false;
        }

        if (::connect(client_socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Connection failed\n";
            return false;
        }

        return true;
    }

    bool sendRequest()
    {
        // 构建HTTP请求
        HttpMessage request;
        request.method = "GET";
        request.path = "/";
        request.version = "HTTP/1.1";
        request.headers["Host"] = host_ + ":" + std::to_string(port_);

        // 如果有存储的cookie，添加到请求头中
        std::string stored_cookie = cookie_manager_.getCookie("session_id");
        if (!stored_cookie.empty())
        {
            request.headers["Cookie"] = "session_id=" + stored_cookie;
        }

        // 构建请求字符串
        std::stringstream ss;
        ss << request.method << " " << request.path << " " << request.version << "\r\n";
        for (const auto &header : request.headers)
        {
            ss << header.first << ": " << header.second << "\r\n";
        }
        ss << "\r\n";

        std::string request_str = ss.str();
        if (send(client_socket_, request_str.c_str(), request_str.length(), 0) < 0)
        {
            std::cerr << "Send failed\n";
            return false;
        }

        return receiveResponse();
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

    bool receiveResponse()
    {
        char buffer[4096];
        std::string response_str;
        int bytes_received;

        while ((bytes_received = recv(client_socket_, buffer, sizeof(buffer) - 1, 0)) > 0)
        {
            buffer[bytes_received] = '\0';
            response_str += buffer;
            if (response_str.find("\r\n\r\n") != std::string::npos)
            {
                break;
            }
        }

        if (bytes_received < 0)
        {
            std::cerr << "Receive failed\n";
            return false;
        }

        // 解析响应中的Set-Cookie头
        std::istringstream iss(response_str);
        std::string line;

        // 跳过状态行
        std::getline(iss, line);
        std::cout << "Response: " << line << std::endl;

        // 解析头部
        while (std::getline(iss, line) && line != "\r" && line != "")
        {
            if (line.substr(0, 11) == "Set-Cookie:")
            {
                std::string cookie_str = line.substr(12);
                size_t semicolon_pos = cookie_str.find(';');
                if (semicolon_pos != std::string::npos)
                {
                    cookie_str = cookie_str.substr(0, semicolon_pos);
                }

                size_t equals_pos = cookie_str.find('=');
                if (equals_pos != std::string::npos)
                {
                    std::string name = cookie_str.substr(0, equals_pos);
                    std::string value = cookie_str.substr(equals_pos + 1);
                    cookie_manager_.setCookie(name, value);
                    std::cout << "Cookie received: " << name << "=" << value << std::endl;
                }
            }
            std::cout << line << std::endl;
        }

        return true;
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

    // 发送三次请求以演示cookie的持久性
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

// 等待一秒后发送下一个请求
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    client.cleanup();
    return 0;
}