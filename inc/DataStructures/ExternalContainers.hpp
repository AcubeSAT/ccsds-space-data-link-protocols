/*
 * @file Containers.hpp
 * @brief Lightweight, templated, heap-free containers that use externally allocated storage. Useful for creating
 *        channels with different sizes of buffers.
 * @note  Always use the isEmpty(), isFull() methods before attempting to pop/push objects from/to a container
 * @warning These containers are not designed for concurrent usage
 */

#pragma once
#include "etl/span.h"

namespace CCSDSDataLinkLayer {
    // Single-ended FIFO queue
    template<typename T>
    class Queue {
    public:
        explicit Queue(etl::span<T> buffer)
            : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {}

        Queue() = default;

        [[nodiscard]] T& getFront() { return buf[tail]; }
        [[nodiscard]] T& getBack() { return buf[(head + capacity - 1) % capacity]; }

        void push(const T &item) {
            buf[head] = item;
            head = (head + 1) % capacity;
            full = (head == tail);
        }

        void push(T &&item) {
            buf[head] = std::move(item);
            head = (head + 1) % capacity;
            full = (head == tail);
        }

        void pop() {
            full = false;
            tail = (tail + 1) % capacity;
        }

        [[nodiscard]] bool isEmpty() const { return (!full && head == tail); }
        [[nodiscard]] bool isFull() const { return full; }

        [[nodiscard]] uint32_t currentSize() const {
            if (full) return capacity;
            if (head >= tail) return head - tail;
            return capacity + head - tail;
        }

        [[nodiscard]] uint32_t maxSize() const { return capacity; }
        [[nodiscard]] uint32_t remainingCapacity() const { return maxSize() - currentSize(); }

        void reset() {
            head = tail = 0;
            full = false;
        }

    private:
        T* buf = nullptr;
        uint32_t capacity = 0;
        uint32_t head = 0;
        uint32_t tail = 0;
        bool full = false;
    };

    // Double-ended queue (deque)
    template<typename T>
    class Dequeue {
    public:
        explicit Dequeue(etl::span<T> buffer)
            : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {}

        Dequeue() = default;

        [[nodiscard]] T& getFront() { return buf[tail]; }

        [[nodiscard]] T& getBack() {
            uint32_t idx = (head + capacity - 1) % capacity;
            return buf[idx];
        }

        void pushBack(const T& item) {
            buf[head] = item;
            head = (head + 1) % capacity;
            full = (head == tail);
        }

        void pushBack(T&& item) {
            buf[head] = std::move(item);
            head = (head + 1) % capacity;
            full = (head == tail);
        }

        void pushFront(const T& item) {
            tail = (tail + capacity - 1) % capacity;
            buf[tail] = item;
            full = (head == tail);
        }

        void pushFront(T&& item) {
            tail = (tail + capacity - 1) % capacity;
            buf[tail] = std::move(item);
            full = (head == tail);
        }

        void popFront() {
            full = false;
            tail = (tail + 1) % capacity;
        }

        void popBack() {
            full = false;
            head = (head + capacity - 1) % capacity;
        }

        [[nodiscard]] bool isEmpty() const { return (!full && head == tail); }
        [[nodiscard]] bool isFull() const { return full; }

        [[nodiscard]] uint32_t currentSize() const {
            if (full) return capacity;
            if (head >= tail) return head - tail;
            return capacity + head - tail;
        }

        [[nodiscard]] uint32_t maxSize() const { return capacity; }
        [[nodiscard]] uint32_t remainingCapacity() const { return maxSize() - currentSize(); }

        void reset() {
            head = tail = 0;
            full = false;
        }

    private:
        T* buf = nullptr;
        uint32_t capacity = 0;
        uint32_t head = 0;
        uint32_t tail = 0;
        bool full = false;
    };

    // Circular buffer (overwrites oldest data when full)
    template<typename T>
    class CircularBuffer {
    public:
        explicit CircularBuffer(etl::span<T> buffer)
            : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {}

        CircularBuffer() = default;

        [[nodiscard]] T& getFront() { return buf[tail]; }

        [[nodiscard]] T& getBack() {
            uint32_t idx = (head + capacity - 1) % capacity;
            return buf[idx];
        }

        void push(const T& item) {
            buf[head] = item;
            if (full) tail = (tail + 1) % capacity;
            head = (head + 1) % capacity;
            full = (head == tail);
        }

        void push(T&& item) {
            buf[head] = std::move(item);
            if (full) tail = (tail + 1) % capacity;
            head = (head + 1) % capacity;
            full = (head == tail);
        }

        void pop() {
            full = false;
            tail = (tail + 1) % capacity;
        }

        [[nodiscard]] bool isEmpty() const { return (!full && head == tail); }
        [[nodiscard]] bool isFull() const { return full; }

        [[nodiscard]] uint32_t currentSize() const {
            if (full) return capacity;
            if (head >= tail) return head - tail;
            return capacity + head - tail;
        }

        [[nodiscard]] uint32_t maxSize() const { return capacity; }
        [[nodiscard]] uint32_t remainingCapacity() const { return maxSize() - currentSize(); }

        void reset() {
            head = tail = 0;
            full = false;
        }

    private:
        T* buf = nullptr;
        uint32_t capacity = 0;
        uint32_t head = 0;
        uint32_t tail = 0;
        bool full = false;
    };

    // Stable unordered pool
    template<typename T>
    class UnorderedPool {
    public:
        explicit UnorderedPool(etl::span<T> buffer, etl::span<uint32_t> freeIndices)
            : buf(buffer.data()), capacity(buffer.size()), sz(0),
              freeIndices(freeIndices.data()), freeCount(buffer.size()) {
            for (uint32_t i = 0; i < capacity; ++i) {
                this->freeIndices[i] = i;
            }
        }

        UnorderedPool() = default;

        [[nodiscard]] T* push(const T& item) {
            uint32_t index = freeIndices[--freeCount];
            buf[index] = item;
            ++sz;
            return &buf[index];
        }

        [[nodiscard]] T* push(T&& item) {
            uint32_t index = freeIndices[--freeCount];
            buf[index] = std::move(item);
            ++sz;
            return &buf[index];
        }

        bool erase(T* item) {
            if (!item || item < buf || item >= buf + capacity) {
                return false;
            }
            uint32_t index = item - buf;
            freeIndices[freeCount++] = index;
            --sz;
            return true;
        }

        [[nodiscard]] bool isEmpty() const { return sz == 0; }
        [[nodiscard]] bool isFull() const { return freeCount == 0; }

        [[nodiscard]] uint32_t currentSize() const { return sz; }
        [[nodiscard]] uint32_t maxSize() const { return capacity; }
        [[nodiscard]] uint32_t remainingCapacity() const { return maxSize() - currentSize(); }

        void reset() {
            sz = 0;
            freeCount = capacity;
            for (uint32_t i = 0; i < capacity; ++i) {
                freeIndices[i] = i;
            }
        }

        class Iterator {
        public:
            Iterator(T* buf, uint32_t* freeIndices, uint32_t freeCount, uint32_t capacity, T* ptr)
                : buf(buf), freeIndices(freeIndices), freeCount(freeCount), capacity(capacity), ptr(ptr) {
                skipToUsed();
            }

            T& operator*() const { return *ptr; }
            T* operator->() const { return ptr; }

            Iterator& operator++() {
                ++ptr;
                skipToUsed();
                return *this;
            }

            bool operator!=(const Iterator& other) const {
                return ptr != other.ptr;
            }

        private:
            void skipToUsed() {
                while (ptr < buf + capacity && isFree(ptr - buf)) {
                    ++ptr;
                }
            }

            bool isFree(uint32_t index) const {
                for (uint32_t i = 0; i < freeCount; ++i) {
                    if (freeIndices[i] == index) {
                        return true;
                    }
                }
                return false;
            }

            T* buf;
            uint32_t* freeIndices;
            uint32_t freeCount;
            uint32_t capacity;
            T* ptr;
        };

        [[nodiscard]] Iterator begin() {
            return Iterator(buf, freeIndices, freeCount, capacity, buf);
        }
        [[nodiscard]] Iterator end() {
            return Iterator(buf, freeIndices, freeCount, capacity, buf + capacity);
        }

    private:
        T* buf = nullptr;
        uint32_t capacity = 0;
        uint32_t sz = 0;
        uint32_t* freeIndices = nullptr;
        uint32_t freeCount = 0;
    };
} // namespace CCSDSDataLinkLayer
