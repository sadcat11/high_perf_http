#include "file_descriptor.hpp"
#include <unistd.h>

namespace hph
{

    FileDescriptor::FileDescriptor(int value) noexcept : value_(value)
    {
    }

    FileDescriptor::~FileDescriptor()
    {
        reset();
    }

    FileDescriptor::FileDescriptor(FileDescriptor &&other) noexcept : value_(other.release())
    {
    }

    FileDescriptor &FileDescriptor::operator=(FileDescriptor &&other) noexcept
    {
        if (this != &other)
        {
            reset(other.release());
        }
        return *this;
    }

    int FileDescriptor::get() const noexcept
    {
        return value_;
    }

    int FileDescriptor::release() noexcept
    {
        const int result = value_;
        value_ = -1;
        return result;
    }

    void FileDescriptor::reset(int value) noexcept
    {
        if (value_ >= 0)
        {
            ::close(value_);
        }
        value_ = value;
    }

}