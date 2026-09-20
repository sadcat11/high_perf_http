#pragma once

namespace hph
{

    class FileDescriptor
    {
    public:
        explicit FileDescriptor(int value = -1) noexcept;
        ~FileDescriptor();

        FileDescriptor(const FileDescriptor &) = delete;
        FileDescriptor &operator=(const FileDescriptor &) = delete;

        FileDescriptor(FileDescriptor &&other) noexcept;
        FileDescriptor &operator=(FileDescriptor &&other) noexcept;

        /**
         * Returns the owned descriptor.
         * @return File descriptor or -1 when empty
         */
        int get() const noexcept;

        /**
         * Releases ownership without closing the descriptor.
         * @return Released descriptor or -1 when empty
         */
        int release() noexcept;

        /**
         * Closes the current descriptor and takes a new one.
         * @param value New descriptor or -1
         * @return No value
         */
        void reset(int value = -1) noexcept;

    private:
        int value_;
    };

}