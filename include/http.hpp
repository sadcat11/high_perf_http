#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace hph
{

    using HeaderMap = std::map<std::string, std::string, std::less<>>;

    struct HttpRequest
    {
        HeaderMap headers;
        std::string method;
        std::string target;
        std::string version;
        std::string body;
    };

    struct HttpResponse
    {
        HeaderMap headers;
        std::uint16_t status = 200;
        std::string reason = "OK";
        std::string body;
    };

    /**
     * Parses one complete HTTP request.
     * @param input Complete request bytes
     * @return Parsed request or empty value when input is invalid or incomplete
     */
    std::optional<HttpRequest> parse_request(std::string_view input);

    /**
     * Serializes a response for a client connection.
     * @param response Response data
     * @return Serialized HTTP response
     */
    std::string serialize_response(const HttpResponse &response);

    /**
     * Creates a plain text error response.
     * @param status HTTP status code
     * @param reason HTTP reason phrase
     * @param body Response body
     * @return Error response
     */
    HttpResponse make_error_response(std::uint16_t status,
                                     std::string reason,
                                     std::string body);

}