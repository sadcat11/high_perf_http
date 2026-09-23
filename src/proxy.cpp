#include "proxy.hpp"
#include "http.hpp"
#include "socket.hpp"
#include "logger.hpp"
#include "http_reader.hpp"
#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <string>
#include <string_view>
#include <unistd.h>
#include <exception>
#include <algorithm>
#include <cctype>

#define DEBUG

#ifdef DEBUG
#include <arpa/inet.h>
#include <netdb.h>
#endif

namespace hph
{
    namespace
    {
#ifdef DEBUG
        std::string client_address_to_string(const sockaddr_storage& address)
        {
            char host[NI_MAXHOST]{};

            const int error = ::getnameinfo(reinterpret_cast<const sockaddr*>(&address),
                                            sizeof(address),
                                            host,
                                            sizeof(host),
                                            nullptr,
                                            0,
                                            NI_NUMERICHOST);

            if (error != 0)
            {
                return "unknown";
            }

            return host;
        }
#endif
        std::string to_lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
                           { return static_cast<char>(std::tolower(character)); });

            return value;
        }

        bool send_all(int fd, std::string_view data)
        {
            std::size_t offset = 0;
            while (offset < data.size())
            {
                const auto count = ::send(fd, data.data() + offset, data.size() - offset, MSG_NOSIGNAL);
                if (count <= 0)
                {
                    return false;
                }
                offset += static_cast<std::size_t>(count);
            }
            return true;
        }

        std::string receive_request(int fd)
        {
            constexpr std::size_t max_request_bytes = 1024 * 1024 + 32 * 1024;
            std::string data;
            std::array<char, 8192> buffer{};
            while (data.size() < max_request_bytes)
            {
                const auto count = ::recv(fd, buffer.data(), buffer.size(), 0);
                if (count <= 0)
                {
                    break;
                }
                data.append(buffer.data(), static_cast<std::size_t>(count));
                if (data.find("\r\n\r\n") != std::string::npos)
                {
                    break;
                }
            }
            return data;
        }

        std::optional<std::string> receive_response(int fd)
        {
            hph::HttpResponseReader reader;

            std::array<char, 8192> buffer{};
            pollfd descriptor{fd, POLLIN, 0};

            while (true)
            {
                const int ready = ::poll(&descriptor, 1, 5000);

                if (ready < 0)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }
                    return std::nullopt;
                }

                if (ready == 0)
                {
                    return std::nullopt;
                }

                if (descriptor.revents & POLLNVAL)
                {
                    return std::nullopt;
                }

                if (descriptor.revents & POLLERR)
                {
                    return std::nullopt;
                }
#ifdef DEBUG
                std::cerr << "poll revents = " << descriptor.revents << "\n";
#endif
                const auto count = ::recv(fd, buffer.data(), buffer.size(), 0);
#ifdef DEBUG
                std::cerr << "recv count = " << count << "\n";
#endif
                if (count < 0)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    return std::nullopt;
                }

                if (count > 0)
                {
                    const auto status = reader.consume(
                        std::string_view(
                            buffer.data(),
                            static_cast<std::size_t>(count)
                        )
                    );
#ifdef DEBUG
                    std::cerr << "reader status = " << static_cast<int>(status) << "\n";
#endif
                    if (status == hph::ReaderStatus::error)
                    {
                        return std::nullopt;
                    }

                    if (status == hph::ReaderStatus::complete)
                    {
                        return reader.take_response();
                    }

                    continue;
                }   

                return std::nullopt;
            }
        }

        void send_error(int client_fd,
                        std::uint16_t status,
                        std::string reason,
                        std::string body)
        {
            const auto response = make_error_response(status,
                                                      std::move(reason),
                                                      std::move(body));

            send_all(client_fd, serialize_response(response));
        }

        void handle_client(int client_fd,
                           const ProxyConfig &config,
                           const sockaddr_storage& client_address)
        {
            FileDescriptor client(client_fd);
#ifdef DEBUG
            const auto client_ip = client_address_to_string(client_address);
#endif
            const auto raw_request = receive_request(client.get());
            const auto request = parse_request(raw_request);

            if (!request)
            {
                send_error(client.get(), 400, "Bad Request", "invalid HTTP request\n");
                return;
            }
#ifdef DEBUG
            hph::log_request(*request, client_ip);
#endif
            FileDescriptor backend = connect_backend(config);

            if (backend.get() < 0)
            {
                send_error(client.get(), 502, "Bad Gateway", "backend unavailable\n");
                return;
            }

            if (!send_all(backend.get(), raw_request))
            {
                send_error(client.get(), 502, "Bad Gateway", "backend write failed\n");
                return;
            }

            const auto response = receive_response(backend.get());

            if (!response)
            {
#ifdef DEBUG
                std::cerr << "receive_response failed\n";
#endif
                send_error(client.get(), 502, "Bad Gateway", "backend response failed\n");
                return;
            }
#ifdef DEBUG
            hph::log_response(*request, *response);
#endif
            send_all(client.get(), *response);
        }

    }

    int run_proxy(const ProxyConfig &config)
    {
        FileDescriptor listener = create_listener(config);
        if (listener.get() < 0)
        {
            std::cerr << "Failed to create listener\n";
            return 1;
        }
        std::cout << "Listening on " << config.listen_address << ":" << config.listen_port << "\n";
        while (true)
        {
            sockaddr_storage address{};
            socklen_t length = sizeof(address);
            const int client = ::accept(listener.get(), reinterpret_cast<sockaddr *>(&address), &length);
            if (client < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                std::cerr << "Accept failed: " << std::strerror(errno) << "\n";
                return 1;
            }
            std::thread(handle_client, client, std::cref(config), address).detach();
        }
    }

}
