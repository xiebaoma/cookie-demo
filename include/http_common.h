#pragma once

#include <string>
#include <map>

// Common structure for HTTP request/response
struct HttpMessage
{
    std::string method; // Request method (used only for requests)
    std::string path;   // Request path (used only for requests)
    std::string version = "HTTP/1.1";
    std::string status_code;    // Status code (used only for responses)
    std::string status_message; // Status message (used only for responses)
    std::map<std::string, std::string> headers;
    std::string body;
};
