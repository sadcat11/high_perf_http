#include "http.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

    void require(bool condition, const char *message)
    {
        if (!condition)
        {
            std::cerr << "Test failed: " << message << "\n";
            std::exit(1);
        }
    }

    void parses_request()
    {
        const auto request = hph::parse_request(
            "POST /hello HTTP/1.1\r\nHost: example.test\r\nContent-Length: 5\r\n\r\nhello");
        require(request.has_value(), "valid request is parsed");
        require(request->method == "POST", "method is parsed");
        require(request->target == "/hello", "target is parsed");
        require(request->headers.at("host") == "example.test", "header is normalized");
        require(request->body == "hello", "body is parsed");
    }

    void rejects_invalid_request()
    {
        require(!hph::parse_request("GET / HTTP/2\r\n\r\n"), "invalid version is rejected");
        require(!hph::parse_request("GET / HTTP/1.1\r\nBroken\r\n\r\n"), "invalid header is rejected");
        require(!hph::parse_request("POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\nabc"), "short body is rejected");
    }

    void serializes_response()
    {
        hph::HttpResponse response;
        response.status = 200;
        response.reason = "OK";
        response.body = "hello";
        response.headers.emplace("content-type", "text/plain");
        const auto raw = hph::serialize_response(response);
        require(raw.find("HTTP/1.1 200 OK\r\n") == 0, "status is serialized");
        require(raw.find("content-length: 5\r\n") != std::string::npos, "length is serialized");
        require(raw.ends_with("\r\n\r\nhello"), "body is serialized");
    }

}

int main()
{
    parses_request();
    rejects_invalid_request();
    serializes_response();
    std::cout << "All tests passed\n";
}
