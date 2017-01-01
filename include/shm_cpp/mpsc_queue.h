
#pragma once

#include <stdalign.h>
#include <atomic>

#include <shm_cpp/segment.h>

namespace shm_cpp {

namespace detail {

const ssize_t cache_line_size = 64;

}

class mpsc_queue
{
public:
    struct header
    {
        ssize_t ring_size; // 8 bytes
        ssize_t slot_size; // 8 bytes
        ssize_t num_slots; // 8 bytes

        char pad[8]; // 8 bytes

        alignas(detail::cache_line_size)  std::atomic<uint64_t> head; // 64 bytes
        alignas(detail::cache_line_size)  std::atomic<uint64_t> tail; // 64 bytes
        alignas(detail::cache_line_size)  std::atomic<uint64_t> ntail; // 64 bytes

        char ring[];
    };

    struct slot
    {
        ssize_t size;
        char body[];
    } __attribute__((packed));

public:
    mpsc_queue() = delete;
    mpsc_queue(const char* queue_name, ssize_t ring_size = 0)
            : m_segment(queue_name, sizeof(header) + ring_size),
              m_hdr(nullptr)
    {}

    ~mpsc_queue()
    {}


    bool create(ssize_t slot_size)
    {
        if (!m_segment.create()) {
            return false;
        }

        m_hdr = (header*)m_segment.get_ptr();
        m_hdr->ring_size = m_segment.get_size() - sizeof(header);
        m_hdr->slot_size = detail::power_of_two(slot_size);
        m_hdr->num_slots = (m_hdr->ring_size / m_hdr->slot_size);

        // mem barrier ? 

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

        // mem barrier? 

        m_hdr = (header*)m_segment.get_ptr();

        return true;
    }

    bool push(const void* data, ssize_t size)
    {
        if (size >= m_hdr->slot_size) {
            return false;
        }

        uint64_t tail = 0;
        do {
            uint64_t head = m_hdr->head.load(std::memory_order_relaxed); 
            head = (head - 1) % m_hdr->num_slots;
            tail = m_hdr->ntail.load(std::memory_order_relaxed);
            if (head == tail) {
                return false;
            }
        } while (!m_hdr->ntail.compare_exchange_strong(tail, (tail + 1 ) % m_hdr->num_slots));

        slot *s= (slot*)&m_hdr->ring[tail * m_hdr->slot_size];
        s->size = size;
        memcpy(s->body, data, size);

        // need mem barrier
        std::atomic_thread_fence(std::memory_order_release);

        // increment the read tail
        do {
            tail = m_hdr->tail;
        } while (!m_hdr->tail.compare_exchange_strong(tail, (tail + 1) % m_hdr->num_slots));

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

        return true;
    }

private:
    segment m_segment;
    header  *m_hdr;
};

} // end namespace shm_cpp
