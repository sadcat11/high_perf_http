#include "http_reader.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <utility>

namespace hph
{
    std::string_view trim_view(std::string_view value)
    {
        while (!value.empty()
               &&
               std::isspace(static_cast<unsigned char>(value.front())))
        {
            value.remove_prefix(1);
        }

        while (!value.empty()
               &&
               std::isspace(static_cast<unsigned char>(value.back())))
        {
            value.remove_suffix(1);
        }

        return value;
    }

    ReaderStatus HttpResponseReader::consume(std::string_view data)
    {
        if (status_ != ReaderStatus::incomplete)
        {
            return status_;
        }

        buffer_.append(data.data(), data.size());

        if (header_end_ == std::string::npos)
        {
            const auto header_status = parse_headers();

            if (header_status == ReaderStatus::error)
            {
                status_ = ReaderStatus::error;
                return status_;
            }

            if (header_end_ == std::string::npos)
            {
                return ReaderStatus::incomplete;
            }
        }

        if (!has_content_length_)
        {
            return ReaderStatus::incomplete;
        }

        const auto body_start = header_end_ + 4;
        const auto body_size = buffer_.size() - body_start;

        if (body_size >= expected_body_size_)
        {
            status_ = ReaderStatus::complete;
        }

        return status_;
    }

    ReaderStatus HttpResponseReader::finish()
    {
        if (status_ != ReaderStatus::incomplete)
        {
            return status_;
        }

        if (header_end_ == std::string::npos)
        {
            status_ = ReaderStatus::error;
            return status_;
        }

        if (!has_content_length_)
        {
            status_ = ReaderStatus::complete;
            return status_;
        }

        const auto body_start = header_end_ + 4;
        const auto body_size = buffer_.size() - body_start;

        if (body_size >= expected_body_size_)
        {
            status_ = ReaderStatus::complete;
        }
        else
        {
            status_ = ReaderStatus::error;
        }

        return status_;
    }

    bool HttpResponseReader::complete() const noexcept
    {
        return status_ == ReaderStatus::complete;
    }

    std::optional<std::string> HttpResponseReader::take_response()
    {
        if (!complete())
        {
            return std::nullopt;
        }

        return std::exchange(buffer_, {});
    }

    ReaderStatus HttpResponseReader::parse_headers()
    {
        header_end_ = buffer_.find("\r\n\r\n");

        if (header_end_ == std::string::npos)
        {
            return ReaderStatus::incomplete;
        }

        const auto headers = std::string_view(buffer_.data(), header_end_);

        if (!parse_content_length(headers))
        {
            return ReaderStatus::error;
        }

        return ReaderStatus::incomplete;
    }

    bool HttpResponseReader::parse_content_length(std::string_view headers)
    {
        std::size_t offset = 0;

        while (offset < headers.size())
        {
            const auto line_end = headers.find("\r\n", offset);

            size_t head_line_len = (line_end == std::string_view::npos) ?
                                    headers.size() - offset : line_end - offset;
            const auto line = headers.substr(offset, head_line_len);

            const auto colon = line.find(':');

            if (colon != std::string_view::npos)
            {
                const auto name = trim_view(line.substr(0, colon));
                const auto value = trim_view(line.substr(colon + 1));

                if (to_lower(std::string(name)) == "content-length")
                {
                    if (value.empty())
                    {
                        return false;
                    }

                    try
                    {
                        std::size_t parsed_characters = 0;

                        expected_body_size_ = std::stoull(std::string(value),
                                                          &parsed_characters);

                        if (parsed_characters != value.size())
                        {
                            return false;
                        }
                    }
                    catch (const std::exception&)
                    {
                        return false;
                    }

                    has_content_length_ = true;
                    return true;
                }
            }

            if (line_end == std::string_view::npos)
            {
                break;
            }

            offset = line_end + 2;
        }

        return true;
    }

    std::string HttpResponseReader::to_lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            }
        );

        return value;
    }
}