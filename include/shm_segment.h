
#pragma once

#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

namespace shm {

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

constexpr uint32_t power_of_2(uint32_t size)
{
    uint32_t i = 0;
    for(; ((uint32_t)10<<i) < size; i++);
    return (uint32_t)10 << i;
}

}



class segment
{
public:
    

public:
    segment() = delete;

    segment(char* name_, uint32_t size_ = 0)
        : m_fd(0),
          m_ptr(nullptr),
          m_name(name_),
          m_size(detail::power_of_2(size_))
    {
    }

    ~segment()
    {
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

        m_ptr = mmap(nullptr, m_size, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);
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
        if (ftruncate(m_fd, m_size) == -1)
            return false;

        m_ptr = mmap(nullptr, m_size, PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0);
        if (m_ptr == nullptr)
            return false;

        return true;
    }

    void unlink()
    {
        ::shm_unlink(m_name.c_str());
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
