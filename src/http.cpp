#include "http.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace hph
{
    namespace
    {

        std::string trim(std::string_view value)
        {
            std::size_t first = 0;
            std::size_t last = value.size();

            while (first < last &&
                   std::isspace(static_cast<unsigned char>(value[first])))
            {
                ++first;
            }

            while (last > first &&
                   std::isspace(static_cast<unsigned char>(value[last - 1])))
            {
                --last;
            }

            return std::string(value.substr(first, last - first));
        }

        std::string lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
                           { return static_cast<char>(std::tolower(character)); });

            return value;
        }

        bool is_token_character(unsigned char character)
        {
            return std::isalnum(character) || character == '-' || character == '_';
        }

        bool valid_token(std::string_view value)
        {
            if (value.empty())
            {
                return false;
            }

            return std::all_of(value.begin(), value.end(), [](unsigned char character)
                               { return is_token_character(character); });
        }

    }

    std::optional<HttpRequest> parse_request(std::string_view input)
    {
        constexpr std::size_t max_header_bytes = 32 * 1024;
        constexpr std::size_t max_body_bytes = 1024 * 1024;

        const auto split = input.find("\r\n\r\n");

        if (split == std::string_view::npos || split > max_header_bytes)
        {
            return std::nullopt;
        }

        const auto header_part = input.substr(0, split);
        const auto body_part = input.substr(split + 4);

        std::istringstream lines{std::string(header_part)};
        std::string line;

        if (!std::getline(lines, line))
        {
            return std::nullopt;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::istringstream request_line{line};
        HttpRequest request;

        if (!(request_line >> request.method >> request.target >> request.version))
        {
            return std::nullopt;
        }

        if (!valid_token(request.method))
        {
            return std::nullopt;
        }

        if (request.target.empty())
        {
            return std::nullopt;
        }

        bool supported_version = (request.version == "HTTP/1.0" || request.version == "HTTP/1.1");

        if (!supported_version)
        {
            return std::nullopt;
        }

        while (std::getline(lines, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            const auto colon = line.find(':');

            if (colon == std::string::npos || colon == 0)
            {
                return std::nullopt;
            }

            const auto name = lower(trim(line.substr(0, colon)));
            const auto value = trim(line.substr(colon + 1));

            if (!valid_token(name))
            {
                return std::nullopt;
            }

            request.headers[name] = value;
        }

        std::size_t content_length = 0;

        const auto it = request.headers.find("content-length");
        if (it != request.headers.end())
        {
            try
            {
                content_length = std::stoull(it->second);
            }
            catch (const std::exception &)
            {
                return std::nullopt;
            }
        }

        if (content_length > max_body_bytes || body_part.size() < content_length)
        {
            return std::nullopt;
        }

        request.body = std::string(body_part.substr(0, content_length));
        return request;
    }

    std::string serialize_response(const HttpResponse &response)
    {
        std::ostringstream output;

        output << "HTTP/1.1 " << response.status << ' ' << response.reason << "\r\n";

        bool has_length = false;

        for (const auto &[name, value] : response.headers)
        {
            if (name == "content-length")
            {
                has_length = true;
            }

            output << name << ": " << value << "\r\n";
        }

        if (!has_length)
        {
            output << "content-length: " << response.body.size() << "\r\n";
        }

        output << "connection: close\r\n\r\n" << response.body;

        return output.str();
    }

    HttpResponse make_error_response(
        std::uint16_t status,
        std::string reason,
        std::string body)
    {
        HttpResponse response;
        response.status = status;
        response.reason = std::move(reason);
        response.body = std::move(body);
        response.headers.emplace("content-type", "text/plain; charset=utf-8");

        return response;
    }

}