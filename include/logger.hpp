#pragma once

#include "http.hpp"

namespace hph
{
    void log_request(const HttpRequest& request, std::string_view client_address);
    void log_response(const HttpRequest& request, std::string_view response_data);
}