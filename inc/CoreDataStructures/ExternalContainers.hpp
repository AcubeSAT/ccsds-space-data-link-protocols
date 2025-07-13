/**
 * @file Containers.hpp
 * @brief Lightweight, templated, heap-free containers that use externally allocated storage. Useful for creating
 *        channels with different sizes of buffers.
 * @note  Always use the isEmpty(), isFull() methods before attempting to pop/push objects from/to a container
 * @warning There containers are not designed for concurrent usage
 */

#pragma once
#include "etl/span.h"

namespace CCSDSDataLinkLayer {
    // Single-ended FIFO queue
    template<typename T>
    class Queue {
    public:
        explicit Queue(etl::span<T> buffer);
        Queue() = default;

        T& getFront();
        void push(const T& item);
        void pop(T& item);

        [[nodiscard]] bool isEmpty() const;
        [[nodiscard]] bool isFull() const;
        [[nodiscard]] size_t currentSize() const;
        [[nodiscard]] size_t maxSize() const;

        void reset();

    private:
        T*      buf;
        size_t  capacity;
        size_t  head;
        size_t  tail;
        bool    full;
    };

    // Double-ended queue (deque)
    template<typename T>
    class Dequeue {
    public:
        explicit Dequeue(etl::span<T> buffer);
        Dequeue() = default;

        T& getFront();
        void pushBack(const T& item);
        void pushFront(const T& item);
        void popFront(T& item);
        void popBack(T& item);

        [[nodiscard]] bool isEmpty() const;
        [[nodiscard]] bool isFull() const;
        [[nodiscard]] size_t currentSize() const;
        [[nodiscard]] size_t maxSize() const;

        void reset();

    private:
        T*      buf;
        size_t  capacity;
        size_t  head;
        size_t  tail;
        bool    full;
    };

    // Circular buffer (overwrites oldest data when full)
    template<typename T>
    class CircularBuffer {
    public:
        explicit CircularBuffer(etl::span<T> buffer);
        CircularBuffer() = default;

        T& getFront();
        void push(const T& item);  // Always succeeds, overwrites if full
        void pop(T& item);

        [[nodiscard]] bool isEmpty() const;
        [[nodiscard]] bool isFull() const;
        [[nodiscard]] size_t currentSize() const;
        [[nodiscard]] size_t maxSize() const;

        void reset();

    private:
        T*      buf;
        size_t  capacity;
        size_t  head;
        size_t  tail;
        bool    full;
    };

    template<typename T>
    class UnorderedPool {
    public:
        explicit UnorderedPool(etl::span<T> buffer);
        UnorderedPool() = default;

        void push(const T& item);
        // return true if erase was successful (false means the item was not contained in the pool)
        bool erase(T& item);
        T& getFront();

        [[nodiscard]] bool   isEmpty()     const;
        [[nodiscard]] bool   isFull()      const;
        [[nodiscard]] size_t currentSize() const;
        [[nodiscard]] size_t maxSize()     const;

        void reset();

    private:
        T*      buf;
        size_t  capacity;
        size_t  sz;
    };

} // namespace CCSDSDataLinkLayer

