#include "internal/LockFreeRingBuffer.h"

#include <fmt/format.h>

#include "thoth/Logger.h"

LockFreeRingBuffer::LockFreeRingBuffer(size_t capacity) : m_buffer(capacity), m_capacity(capacity) {
    // avoid m_readIdx = 0 that equals to store(0, std::memory_order_seq_cst) (lowest performance)
    m_readIdx.store(0);
    m_writeIdx.store(0);
}

size_t LockFreeRingBuffer::write(const char* data, size_t size) {
    size_t curR = m_readIdx.load(std::memory_order_acquire);
    size_t curW = m_writeIdx.load(std::memory_order_relaxed);

    size_t availableSpace = availableWriteSpace(curR, curW);
    if (availableSpace == 0) {
        LOG_WARN("Ring buffer is full, skipping write operation");
        return 0;
    }
    size_t toWrite = std::min(size, availableSpace);
    size_t sizeToTail = std::min(toWrite, m_capacity - curW);
    std::memcpy(&m_buffer[curW], data, sizeToTail);
    if (toWrite > sizeToTail) {
        // Wrap around the buffer
        size_t sizeFromStart = toWrite - sizeToTail;
        std::memcpy(&m_buffer[0], data + sizeToTail, sizeFromStart);
    }

    // ATOMIC OP, update the write index
    // std::memory_order_release -> ensure when consumer see the writeIdx is updated, the data is
    // already written
    size_t nextWrite = (curW + toWrite) % m_capacity;
    m_writeIdx.store(nextWrite, std::memory_order_release);

    return toWrite;
}

size_t LockFreeRingBuffer::read(char* dest, size_t size) {
    size_t curR = m_readIdx.load(std::memory_order_relaxed);
    size_t curW = m_writeIdx.load(std::memory_order_acquire);

    size_t availableSpace = availableReadSpace(curR, curW);
    if (availableSpace == 0) {
        LOG_TRACE("Ring buffer is empty, skipping read operation");
        return 0;
    }

    size_t toRead = std::min(size, availableSpace);
    size_t sizeToTail = std::min(toRead, m_capacity - curR);
    std::memcpy(dest, &m_buffer[curR], sizeToTail);
    if (toRead > sizeToTail) {
        // Wrap around the buffer
        size_t sizeFromStart = toRead - sizeToTail;
        std::memcpy(dest + sizeToTail, &m_buffer[0], sizeFromStart);
    }

    // ATOMIC OP, update the read index
    size_t nextRead = (curR + toRead) % m_capacity;
    m_readIdx.store(nextRead, std::memory_order_release);

    return toRead;
}

size_t LockFreeRingBuffer::size() const {
    size_t curR = m_readIdx.load(std::memory_order_acquire);
    size_t curW = m_writeIdx.load(std::memory_order_acquire);
    return availableReadSpace(curR, curW);
}

size_t LockFreeRingBuffer::capacity() const { return m_capacity; }

bool LockFreeRingBuffer::isEmpty() const { return size() == 0; }

bool LockFreeRingBuffer::isFull() const {
    size_t curR = m_readIdx.load(std::memory_order_acquire);
    size_t curW = m_writeIdx.load(std::memory_order_acquire);
    return availableWriteSpace(curR, curW) == 0;
}

void LockFreeRingBuffer::clear() {
    m_readIdx.store(0);
    m_writeIdx.store(0);
}

size_t LockFreeRingBuffer::availableReadSpace(size_t r, size_t w) const {
    if (w >= r) {
        return w - r;
    }
    return m_capacity - r + w;
}

size_t LockFreeRingBuffer::availableWriteSpace(size_t r, size_t w) const {
    size_t capacity = m_capacity;
    size_t readSpace = availableReadSpace(r, w);
    return capacity - readSpace - FULL_MARGIN;
}

std::ostream& operator<<(std::ostream& os, const LockFreeRingBuffer& buffer) {
    size_t r = buffer.m_readIdx.load(std::memory_order_relaxed);
    size_t w = buffer.m_writeIdx.load(std::memory_order_relaxed);

    os << "LockFreeRingBuffer(capacity=" << buffer.m_capacity << ", readIdx=" << r
       << ", writeIdx=" << w << ", buffer=";

    for (size_t i = 0; i < buffer.m_buffer.size(); ++i) {
        if (i > 0) os << ", ";
        os << static_cast<int>(buffer.m_buffer[i]);
    }
    os << ")";
    return os;
}
