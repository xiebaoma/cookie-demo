#pragma once
#include <string>
#include <map>

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