
#pragma once

#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

namespace shm_cpp {

namespace detail {

enum class oflags : uint32_t
{
    create_only = O_CREAT | O_EXCL, 
    read_write  = O_RDWR,
    read_only   = O_RDONLY
};

inline oflags operator |(oflags lhs, oflags rhs)
{
    return static_cast<oflags>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr uint32_t power_of_two(uint32_t size)
{
    uint32_t po2 = 1;
    while (po2 < size) {
        po2 <<= 1;
    }
    return po2;
}

}


class segment
{
public:
    segment() = delete;

    segment(const char* name_, size_t size_ = 0)
        : m_fd(0),
          m_ptr(nullptr),
          m_name(name_),
          m_size(detail::power_of_two(size_))
    {
    }

    ~segment()
    {
        this->close();
    }

    inline void* get_ptr() const
    {
        return m_ptr;
    }

    inline size_t get_size() const
    {
        return m_size;
    }

    bool create(detail::oflags flags = detail::oflags::create_only | detail::oflags::read_write)
    {
        m_fd = ::shm_open(m_name.c_str(), 
                static_cast<int>(flags),
                S_IRUSR|S_IWUSR);

        if (m_fd == -1) 
            return false;

        if (ftruncate(m_fd, m_size) == -1)
            return false;

        m_ptr = mmap64(nullptr, m_size, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);
        if (m_ptr == nullptr) 
            return false;

        memset(m_ptr, 0, m_size);

        return true;
    }

    bool open()
    {
        m_fd = ::shm_open(m_name.c_str(), 
                          static_cast<int>(detail::oflags::read_write),
                          S_IRUSR|S_IWUSR);


        struct stat file;
        if (fstat(m_fd, &file) == -1)
            return false;

        m_size = file.st_size;

        m_ptr = mmap(nullptr, m_size, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);
        if (m_ptr == nullptr)
            return false;

        return true;
    }

    bool unlink()
    {
        return (::shm_unlink(m_name.c_str()) == 0);
    }

    void close()
    {
        if (m_fd != 0) {
            ::close(m_fd);
        }
    }

private:
    int             m_fd;
    void*           m_ptr;
    std::string     m_name;
    uint32_t        m_size;
};

} //end namespace shm
