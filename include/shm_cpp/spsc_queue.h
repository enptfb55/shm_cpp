
#pragma once

#include <stdalign.h>
#include <atomic>

#include <shm_cpp/segment.h>

namespace shm_cpp {

namespace detail {

const ssize_t cache_line_size = 64;

}

class spsc_queue
{
private:
    struct header
    {
        ssize_t ring_size;
        ssize_t slot_size;
        ssize_t num_slots;

        char pad[8];

        alignas(detail::cache_line_size) volatile uint64_t head;
        alignas(detail::cache_line_size) volatile uint64_t tail;

        char ring[];
    } __attribute__((packed));

    struct slot
    {
        ssize_t size;
        char body[];
    } __attribute__((packed));

public:
    spsc_queue() = delete;
    spsc_queue(const char* queue_name, ssize_t ring_size = 0)
            : m_segment(queue_name, sizeof(header) + ring_size),
              m_hdr(nullptr)
    {}

    ~spsc_queue()
    {}


    bool create(ssize_t slot_size)
    {
        if (!m_segment.create()) {
            return false;
        }

        m_hdr = (header*)m_segment.get_ptr();
        m_hdr->ring_size = m_segment.get_size() - sizeof(header);
        m_hdr->slot_size = slot_size;
        m_hdr->num_slots = (m_hdr->ring_size / m_hdr->slot_size);

        return true;
    }

    bool destroy() {
        return m_segment.unlink();
    }

    bool open()
    {
        if (!m_segment.open()) {
            return false;
        }

        m_hdr = (header*)m_segment.get_ptr();
        m_hdr->ring_size = m_segment.get_size() - sizeof(header);

        return true;
    }

    bool push(const void* data, ssize_t size)
    {
        if (size >= m_hdr->slot_size) {
            return false;
        }

        uint32_t head = (m_hdr->head - 1) % m_hdr->num_slots;
        uint32_t tail = m_hdr->tail;
        if (head == tail) {
            return false;
        }

        slot *s= (slot*)&m_hdr->ring[tail * m_hdr->slot_size];
        s->size = size;
        memcpy(s->body, data, size);

        // need mem barrier
        std::atomic_thread_fence(std::memory_order_release);

        m_hdr->tail = (tail + 1) % m_hdr->num_slots;

        //printf("tail is  %lu\n", m_hdr->tail);
        return true;
    }

    bool pop(void* data, ssize_t* size) 
    {
        uint32_t head = m_hdr->head;
        uint32_t tail = m_hdr->tail;
        if (head == tail) {
            return false;
        }

        slot* s = (slot*)&m_hdr->ring[head * m_hdr->slot_size];
        *size = s->size;
        memcpy(data, s->body, s->size);

        // need mem barrier
        std::atomic_thread_fence(std::memory_order_acquire);

        m_hdr->head = (head + 1) % m_hdr->num_slots;

        //printf("head is  %lu\n", m_hdr->head);

        return true;
    }

private:
    segment m_segment;
    header  *m_hdr;
};

} // end namespace shm_cpp
