#include "proxy.hpp"
#include <cstdint>
#include <string>

int main(int argc, char **argv)
{
    hph::ProxyConfig config;
    if (argc > 1)
        config.listen_port = static_cast<std::uint16_t>(std::stoul(argv[1]));
    if (argc > 2)
        config.backend_port = static_cast<std::uint16_t>(std::stoul(argv[2]));
    if (argc > 3)
        config.backend_host = argv[3];

    return hph::run_proxy(config);
}
