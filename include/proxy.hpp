#pragma once

#include <cstdint>
#include <string>

namespace hph
{

    struct ProxyConfig
    {
        std::string listen_address = "127.0.0.1";
        std::uint16_t listen_port = 8080;
        std::string backend_host = "127.0.0.1";
        std::uint16_t backend_port = 18080;
    };

    /**
     * Runs the blocking proxy server.
     * @param config Listener and backend configuration
     * @return Zero on normal stop or nonzero on startup error
     */
    int run_proxy(const ProxyConfig &config);

}