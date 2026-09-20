#include "socket.hpp"
#include <netdb.h>
#include <sys/socket.h>
#include <string>

namespace hph
{

    FileDescriptor connect_backend(const ProxyConfig &config)
    {
        const auto service = std::to_string(config.backend_port);

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo *result = nullptr;
        if (::getaddrinfo(config.backend_host.c_str(), service.c_str(), &hints, &result) != 0)
        {
            return {};
        }

        FileDescriptor socket;

        for (auto *item = result; item != nullptr; item = item->ai_next)
        {
            socket.reset(::socket(item->ai_family, item->ai_socktype, item->ai_protocol));

            if (socket.get() >= 0
                &&
                ::connect(socket.get(), item->ai_addr, item->ai_addrlen) == 0)
            {
                ::freeaddrinfo(result);
                return socket;
            }
        }

        ::freeaddrinfo(result);
        return {};
    }

    FileDescriptor create_listener(const ProxyConfig &config)
    {
        const auto service = std::to_string(config.listen_port);

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        addrinfo *result = nullptr;
        if (::getaddrinfo(config.listen_address.c_str(), service.c_str(), &hints, &result) != 0)
        {
            return {};
        }

        FileDescriptor listener;

        for (auto *item = result; item != nullptr; item = item->ai_next)
        {
            listener.reset(::socket(item->ai_family, item->ai_socktype, item->ai_protocol));

            if (listener.get() < 0)
            {
                continue;
            }

            int enabled = 1;

            ::setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

            if (::bind(listener.get(), item->ai_addr, item->ai_addrlen) == 0
                &&
                ::listen(listener.get(), 512) == 0)
            {
                ::freeaddrinfo(result);
                return listener;
            }
        }

        ::freeaddrinfo(result);
        return {};
    }

}