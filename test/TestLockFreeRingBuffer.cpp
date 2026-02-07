#include <gtest/gtest.h>

#include <atomic>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include "internal/LockFreeRingBuffer.h"
#include "thoth/Logger.h"

class LockFreeRingBufferTest : public ::testing::Test {
   protected:
    std::vector<char> generateRandomData(size_t size) {
        std::vector<char> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<char>(i % MOD);
        }
        return data;
    }

   private:
    static constexpr size_t MOD = 256;
};

// Single Thread Test

TEST_F(LockFreeRingBufferTest, ReadWriteInSingleThread) {
    size_t capacity = 1024;
    LockFreeRingBuffer buffer(capacity);

    EXPECT_TRUE(buffer.isEmpty());
    EXPECT_EQ(buffer.size(), 0);

    std::string input = "I LOVE Mayday!";
    size_t n_written = buffer.write(input.data(), input.size());
    EXPECT_EQ(n_written, input.size());
    EXPECT_FALSE(buffer.isEmpty());
    EXPECT_EQ(buffer.size(), input.size());

    char output[1024];
    size_t n_read = buffer.read(output, input.size());
    EXPECT_EQ(n_read, input.size());

    std::string result(output, n_read);
    EXPECT_EQ(result, input);
    EXPECT_TRUE(buffer.isEmpty());
    EXPECT_EQ(buffer.size(), 0);
}

TEST_F(LockFreeRingBufferTest, FullCapacityBehavior) {
    size_t buffer_capacity = 10;
    size_t input_size = 20;
    LockFreeRingBuffer buffer(buffer_capacity);

    std::vector<char> input(input_size, 'A');
    size_t n_written = buffer.write(input.data(), input.size());
    EXPECT_EQ(n_written, buffer_capacity - 1);
    EXPECT_EQ(buffer.size(), buffer_capacity - 1);  // 1 byte is reserved for full check

    size_t n_written_2 = buffer.write(input.data(), input.size());
    EXPECT_EQ(n_written_2, 0);  // buffer is full, no data is written
}

TEST_F(LockFreeRingBufferTest, WrapAroundBehavior) {
    size_t buffer_capacity = 10;
    LockFreeRingBuffer buffer(buffer_capacity);

    // [] -> [0, 1, 2, 3, 4, 0, 0, 0, 0, 0], R = 0, W = 5
    //       [R,  ,  ,  ,  , W,  ,  ,  ,  ]
    std::vector<char> input_1 = generateRandomData(5);
    size_t n_written_1 = buffer.write(input_1.data(), input_1.size());
    EXPECT_EQ(n_written_1, 5);
    EXPECT_EQ(buffer.size(), 5);
    std::cout << buffer << std::endl;

    // [0, 1, 2, 3, 4, 0, 0, 0, 0, 0], R = 3, W = 5
    // [ ,  ,  , R,  , W,  ,  ,  ,  ]
    char output_1[10];
    size_t n_exp_output_1 = 3;
    size_t n_read_1 = buffer.read(output_1, n_exp_output_1);  // [0, 1, 2]
    EXPECT_EQ(n_read_1, n_exp_output_1);
    EXPECT_EQ(buffer.size(), 2);

    // [A, A, 2, 3, 4, A, A, A, A, A], R = 3, W = 2
    // [ ,  , W, R,  ,  ,  ,  ,  ,  ]
    std::vector<char> input_2(7, 'A');
    size_t n_written_2 = buffer.write(input_2.data(), input_2.size());
    EXPECT_EQ(n_written_2, 7);
    EXPECT_EQ(buffer.size(), 9);  // 2(old) + 7(new)

    char output_2[10];
    size_t n_exp_output_2 = 8;
    size_t n_read_2 = buffer.read(output_2, n_exp_output_2);
    EXPECT_EQ(n_read_2, n_exp_output_2);
    EXPECT_EQ(buffer.size(), 1);

    std::vector<char> expected = {3, 4, 'A', 'A', 'A', 'A', 'A', 'A'};
    std::vector<char> result_2(output_2, output_2 + n_read_2);
    EXPECT_EQ(result_2, expected);
}

// Multi-Thread Test, pressure test

TEST_F(LockFreeRingBufferTest, SPSC_PressureTest) {
    const size_t kBufferSize = 4096;                  // 4KB
    const size_t kTotalDataBytes = 10 * 1024 * 1024;  // 10MB
    const size_t kChunkSize = 1024;                   // 1KB
    LockFreeRingBuffer buffer(kBufferSize);

    std::atomic<bool> producerFinished{false};

    std::thread consumerThread([&]() {
        std::vector<unsigned char> receivedData;
        receivedData.reserve(kTotalDataBytes);

        char tempBuf[kChunkSize];
        while (true) {
            size_t bytesToRead = rand() % sizeof(tempBuf) + 1;
            size_t nRead = buffer.read(tempBuf, bytesToRead);

            if (nRead > 0) {
                receivedData.insert(receivedData.end(), tempBuf, tempBuf + nRead);
                continue;
            }

            if (producerFinished.load(std::memory_order_acquire)) {
                // double check, producer might have finished but the buffer is not empty yet
                if (buffer.isEmpty()) break;
            } else {
                std::this_thread::yield();
            }
        }

        ASSERT_EQ(receivedData.size(), kTotalDataBytes) << "Data length MISMATCH";

        for (size_t i = 0; i < kTotalDataBytes; ++i) {
            unsigned char expected = i % 256;
            if (receivedData[i] != expected) {
                FAIL() << "Data mismatch at index " << i << ", expected " << expected << ", got "
                       << receivedData[i];
            }
        }
    });

    std::thread producerThread([&]() {
        std::vector<char> chunk(kChunkSize >> 1);  // 512 bytes
        size_t bytesGenerated = 0;

        while (bytesGenerated < kTotalDataBytes) {
            size_t validBytes = 0;
            for (size_t i = 0; i < chunk.size() && bytesGenerated < kTotalDataBytes; ++i) {
                chunk[i] = static_cast<char>(bytesGenerated % 256);
                bytesGenerated++;
                validBytes++;
            }

            size_t offset = 0;
            size_t bytesToWrite = validBytes;
            while (bytesToWrite > 0) {
                size_t nWrite = buffer.write(chunk.data() + offset, bytesToWrite);
                if (nWrite > 0) {
                    offset += nWrite;
                    bytesToWrite -= nWrite;
                } else {
                    // buffer is full, wait for the consumer to read
                    std::this_thread::yield();
                }
            }
        }
        LOG_INFO("Produced {} bytes", bytesGenerated);
        producerFinished.store(true, std::memory_order_release);
    });

    if (producerThread.joinable()) producerThread.join();
    if (consumerThread.joinable()) consumerThread.join();
    SUCCEED();
}