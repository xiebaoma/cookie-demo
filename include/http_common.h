#pragma once

#include <string>
#include <map>

// HTTP请求/响应的通用结构
struct HttpMessage {
    std::string method;              // 请求方法（仅用于请求）
    std::string path;                // 请求路径（仅用于请求）
    std::string version = "HTTP/1.1";
    std::string status_code;         // 状态码（仅用于响应）
    std::string status_message;      // 状态消息（仅用于响应）
    std::map<std::string, std::string> headers;
    std::string body;
};

// Cookie管理类
class CookieManager {
public:
    void setCookie(const std::string& name, const std::string& value) {
        cookies[name] = value;
    }

    std::string getCookie(const std::string& name) const {
        auto it = cookies.find(name);
        return it != cookies.end() ? it->second : "";
    }

    std::map<std::string, std::string> getAllCookies() const {
        return cookies;
    }

private:
    std::map<std::string, std::string> cookies;
};