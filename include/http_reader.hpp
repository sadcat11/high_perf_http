#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace hph
{
    enum class ReaderStatus
    {
        incomplete,
        complete,
        error
    };

    class HttpResponseReader
    {
    public:
        /**
         * Adds bytes to the response parser.
         * @param data Received response bytes
         * @return Current parser status
         */
        ReaderStatus consume(std::string_view data);

        /**
         * Finishes a close-delimited response.
         * @return Current parser status
         */
        ReaderStatus finish();

        /**
         * Checks whether the response is complete.
         * @return True when the response is complete
         */
        bool complete() const noexcept;

        /**
         * Returns the complete response and clears the reader.
         * @return Complete response or empty value
         */
        std::optional<std::string> take_response();

    private:
        /**
         * Finds the end of response headers.
         * @return Current parser status
         */
        ReaderStatus parse_headers();

        /**
         * Reads Content-Length header.
         * @param headers Response headers without final separator
         * @return True when header is valid or absent
         */
        bool parse_content_length(std::string_view headers);

        /**
         * Converts ASCII letters to lower case.
         * @param value Input text
         * @return Lower case text
         */
        static std::string to_lower(std::string value);

        std::string buffer_;
        std::size_t header_end_ = std::string::npos;
        std::size_t expected_body_size_ = 0;
        bool has_content_length_ = false;
        ReaderStatus status_ = ReaderStatus::incomplete;
    };
}