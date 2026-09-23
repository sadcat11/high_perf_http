#include "logger.hpp"
#include <mutex>
#include <iostream>

namespace hph
{
    namespace
    {
        std::mutex log_mutex;
    }

    void log_request(const HttpRequest& request, std::string_view client_address)
    {
        std::mutex log_mutex;
        std::lock_guard<std::mutex> lock(log_mutex);

        std::cout << "log_request:\n" << client_address << " "
                  << request.method << " " << request.target << "\n";
    }

    void log_response(const HttpRequest& request, std::string_view response_data)
    {
        std::mutex log_mutex;
        const auto line_end = response_data.find("\r\n");

        if (line_end == std::string_view::npos)
        {
            std::cout << "log_response: line_end == std::string_view::npos\n";
            return;
        }

        std::lock_guard<std::mutex> lock(log_mutex);

        std::cout << "log_response:\n" << request.method << " target: " << request.target
                << " data: " << response_data.substr(0, line_end) << "\n";
    }
}