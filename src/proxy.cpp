#include "proxy.hpp"
#include "http.hpp"
#include "socket.hpp"
#include "logger.hpp"
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
                const auto response = make_error_response(400,
                                                          "Bad Request",
                                                          "invalid HTTP request\n");

                send_all(client.get(), serialize_response(response));
                return;
            }
#ifdef DEBUG
            hph::log_request(*request, client_ip);
#endif
            FileDescriptor backend = connect_backend(config);

            if (backend.get() < 0)
            {
                const auto response = make_error_response(502,
                                                          "Bad Gateway",
                                                          "backend unavailable\n");

                send_all(client.get(), serialize_response(response));
                return;
            }

            if (!send_all(backend.get(), raw_request))
            {
                const auto response = make_error_response(502,
                                                          "Bad Gateway",
                                                          "backend write failed\n");

                send_all(client.get(), serialize_response(response));
                return;
            }

            std::string response_data;
            std::array<char, 8192> buffer{};
            pollfd descriptor{backend.get(), POLLIN, 0};

            std::size_t header_end = std::string::npos;
            std::size_t expected_body_size = 0;
            bool has_content_length = false;

            while (true)
            {
                const int ready = ::poll(&descriptor, 1, 5000);

                if (ready <= 0 || !(descriptor.revents & POLLIN))
                {
                    break;
                }

                const auto count = ::recv(backend.get(), buffer.data(), buffer.size(), 0);

                if (count <= 0)
                {
                    break;
                }

                response_data.append(buffer.data(), static_cast<std::size_t>(count));

                if (header_end == std::string::npos)
                {
                    header_end = response_data.find("\r\n\r\n");

                    if (header_end == std::string::npos)
                    {
                        continue;
                    }

                    const auto headers = response_data.substr(0, header_end);
                    const auto lower_headers = to_lower(headers);
                    const auto length_start = lower_headers.find("content-length:");

                    if (length_start != std::string::npos)
                    {
                        const auto value_start = length_start + 15;
                        const auto value_end = lower_headers.find("\r\n", value_start);

                        if (value_end != std::string::npos)
                        {
                            try
                            {
                                expected_body_size =
                                    std::stoull(headers.substr(value_start,
                                                               value_end - value_start));

                                has_content_length = true;
                            }
                            catch (const std::exception &)
                            {
                                break;
                            }
                        }
                    }
                }

                if (header_end == std::string::npos)
                {
                    continue;
                }

                const auto body_start = header_end + 4;
                const auto received_body_size = response_data.size() - body_start;

                if (has_content_length && received_body_size >= expected_body_size)
                {
                    break;
                }
            }

            if (!response_data.empty())
            {
#ifdef DEBUG
                hph::log_response(*request, response_data);
#endif
                send_all(client.get(), response_data);
            }
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
