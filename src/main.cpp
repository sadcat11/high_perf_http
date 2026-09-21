#include "proxy.hpp"
#include <cstdint>
#include <string>
#include <iostream>
#include <limits>
#include <exception>
#include <stdexcept>
#include <netdb.h>

void print_available_ports()
{
    std::cerr << "Available ports: 1024-49151\n";
}

void print_invalid_port()
{
    std::cerr << "Invalid port\n";
    print_available_ports();
}

bool check_port(const char *str) {
    if (str == nullptr || *str == '\0')
    {
        print_invalid_port();
        return false;
    }
    char *endptr = nullptr;
    long number = std::strtol(str, &endptr, 10);

    if (*endptr != '\0' || errno == ERANGE)
    {
        print_invalid_port();
        return false;
    }

    /* Not system or dynamic port */
    if (number < 1024 && number > 49151)
    {
        print_invalid_port();
        return false;
    }

    return true;
}

bool check_host(const char *str)
{
    if (str == nullptr || *str == '\0')
    {
        std::cerr << "Invalid backend host\n";
        return false;
    }

    addrinfo address_filter{};
    address_filter.ai_family = AF_UNSPEC;
    address_filter.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    const int error = ::getaddrinfo(str, nullptr, &address_filter, &result);

    if (error != 0) {
        std::cerr << "Invalid backend host <" << str << ">: "
                  << ::gai_strerror(error) << "\n";

        return false;
    }

    ::freeaddrinfo(result);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        std::cerr << "Incorrect arguments, try:\n"
                  << argv[0] << " <port> <source_port> <host>\n";
        print_available_ports();
        return -1;
    }

    hph::ProxyConfig config;

    if (check_port(argv[1]))
    {
        config.listen_port = static_cast<std::uint16_t>(std::stoul(argv[1]));
    }
    else
    {
        return -1;
    }

    if (check_port(argv[2]))
    {
        config.backend_port = static_cast<std::uint16_t>(std::stoul(argv[2]));
    }
    else
    {
        return -1;
    }

    if (check_host(argv[3]))
    {
        config.backend_host = argv[3];
    }
    else
    {
        return -1;
    }

    return hph::run_proxy(config);
}
