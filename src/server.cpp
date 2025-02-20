#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
// 在现有的include之后添加
#include <sstream>
#include <cstdlib>
#include <ctime>
#endif

#include <iostream>
#include <string>
#include <sstream> // 添加这行
#include <cstdlib>
#include <ctime>
#include "http_common.h"
#include "cookie_manager.h"

class HttpServer
{
public:
    HttpServer(int port) : port_(port) {}

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
        // 创建socket
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0)
        {
            std::cerr << "Socket creation failed\n";
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port_);

        if (bind(server_socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            std::cerr << "Bind failed\n";
            return false;
        }

        if (listen(server_socket_, 5) < 0)
        {
            std::cerr << "Listen failed\n";
            return false;
        }

        std::cout << "Server listening on port " << port_ << std::endl;
        return true;
    }

    void run()
    {
        while (true)
        {
            sockaddr_in client_addr{};
            int client_len = sizeof(client_addr);

#ifdef _WIN32
            SOCKET client_socket = accept(server_socket_, (struct sockaddr *)&client_addr, &client_len);
            if (client_socket == INVALID_SOCKET)
#else
            int client_socket = accept(server_socket_, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
            if (client_socket < 0)
#endif
            {
                std::cerr << "Accept failed\n";
                continue;
            }

            handleClient(client_socket);
        }
    }

private:
#ifdef _WIN32
    SOCKET server_socket_;
#else
    int server_socket_;
#endif
    int port_;
    CookieManager cookie_manager_;

    // 解析HTTP请求
    HttpMessage parseRequest(const std::string &request_str)
    {
        HttpMessage request;
        std::istringstream iss(request_str);
        std::string line;

        // 解析请求行
        std::getline(iss, line);
        std::istringstream request_line(line);
        request_line >> request.method >> request.path >> request.version;

        // 解析请求头
        while (std::getline(iss, line) && line != "\r" && line != "")
        {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 2); // 跳过": "
                if (!value.empty() && value.back() == '\r')
                {
                    value.pop_back();
                }
                request.headers[key] = value;
            }
        }

        return request;
    }

    // 生成HTTP响应
    std::string generateResponse(const HttpMessage &request)
    {
        HttpMessage response;
        response.version = "HTTP/1.1";
        response.status_code = "200";
        response.status_message = "OK";

        // 设置一个新的cookie
        std::string session_id = "user_" + std::to_string(std::rand());
        response.headers["Set-Cookie"] = "session_id=" + session_id + "; Max-Age=3600; Path=/";
        response.headers["Content-Type"] = "text/plain";

        // 检查请求中的cookie
        auto it = request.headers.find("Cookie");
        std::string response_body;
        if (it != request.headers.end())
        {
            response_body = "Welcome back! Your cookie: " + it->second + "\n";
        }
        else
        {
            response_body = "Hello! This is your first visit.\n";
        }

        response.body = response_body;
        response.headers["Content-Length"] = std::to_string(response_body.length());

        // 构建响应字符串
        std::stringstream ss;
        ss << response.version << " " << response.status_code << " " << response.status_message << "\r\n";
        for (const auto &header : response.headers)
        {
            ss << header.first << ": " << header.second << "\r\n";
        }
        ss << "\r\n"
           << response.body;

        return ss.str();
    }

    void handleClient(int client_socket)
    {
        char buffer[4096] = {0};
        std::string request_str;

        // 接收请求
        int bytes_received;
        while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0)
        {
            buffer[bytes_received] = '\0';
            request_str += buffer;
            if (request_str.find("\r\n\r\n") != std::string::npos)
            {
                break;
            }
        }

        if (bytes_received > 0)
        {
            // 解析请求并生成响应
            HttpMessage request = parseRequest(request_str);
            std::string response = generateResponse(request);

            // 发送响应
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
    std::srand(std::time(nullptr)); // 初始化随机数种子
    HttpServer server(8080);
    if (!server.init())
    {
        return 1;
    }
    server.run();
    return 0;
}