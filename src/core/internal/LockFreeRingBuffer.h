#pragma once

#include <fmt/ostream.h>

#include <atomic>
#include <cstdint>
#include <cstring>  // for memcpy
#include <vector>

/*
A lock-free RING buffer, espcially used in Single-Producer Single-Consumer (SPSC) scenarios.
    The "waste one slot" strategy is used to simplify the implementation.
        - Empty: R == W
        - Writing: W is moving forward
        - Full: (W + 1) % capacity == R
    The Single-Producer Single-Consumer (SPSC) pattern:
        - Capture Thread (Producer): move forward the writeIdx, read the readIdx
        - Storage Thread (Consumer): move forward the readIdx, read the writeIdx
        there is no variable shared between the two threads, so no need to use mutex.
    std::memory_order and std::atomic to force thread safe operations
        - std::memory_order_release used when write, fill in data then move the pointer
        - std::memory_order_acquire, used when read, see the pointer update then read data
    Cache line splitting is used to avoid false sharing.
*/
class LockFreeRingBuffer {
   public:
    explicit LockFreeRingBuffer(size_t capacity);

    // Forbid copy
    LockFreeRingBuffer(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer& operator=(const LockFreeRingBuffer&) = delete;

    // Write data to the buffer, called by the producer
    size_t write(const char* data, size_t size);
    // Read data from the buffer, called by the consumer
    size_t read(char* dest, size_t size);

    size_t size() const;
    bool isEmpty() const;
    bool isFull() const;
    size_t capacity() const;
    void clear();

    friend std::ostream& operator<<(std::ostream& os, const LockFreeRingBuffer& buffer);

   private:
    std::vector<char> m_buffer;
    size_t m_capacity;

    // Aligned to 64 bytes for splitting the cache line, to avoid false sharing
    alignas(64) std::atomic<size_t> m_readIdx;
    alignas(64) std::atomic<size_t> m_writeIdx;

    size_t availableReadSpace(size_t r, size_t w) const;
    size_t availableWriteSpace(size_t r, size_t w) const;

    static constexpr size_t FULL_MARGIN = 1;
};

template <>
struct fmt::formatter<LockFreeRingBuffer> : fmt::ostream_formatter {};
