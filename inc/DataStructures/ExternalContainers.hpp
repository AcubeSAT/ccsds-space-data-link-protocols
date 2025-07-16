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
        explicit Queue(etl::span<T> buffer);
        Queue() = default;

        T& getFront();
        T& getBack();
        void push(const T& item);
        void push(T&& item);
        void pop();

        [[nodiscard]] bool isEmpty() const;
        [[nodiscard]] bool isFull() const;
        [[nodiscard]] uint32_t currentSize() const;
        [[nodiscard]] uint32_t maxSize() const;
        [[nodiscard]] uint32_t remainingCapacity() const;

        void reset();

    private:
        T*      buf = nullptr;
        uint32_t  capacity = 0;
        uint32_t  head = 0;
        uint32_t  tail = 0;
        bool    full = false;
    };

    // Double-ended queue (deque)
    template<typename T>
    class Dequeue {
    public:
        explicit Dequeue(etl::span<T> buffer);
        Dequeue() = default;

        T& getFront();
        T& getBack();

        void pushBack(const T& item);
        void pushBack(T&& item);
        void pushFront(const T& item);
        void pushFront(T&& item);

        void popFront();
        void popBack();

        [[nodiscard]] bool isEmpty() const;
        [[nodiscard]] bool isFull() const;
        [[nodiscard]] uint32_t currentSize() const;
        [[nodiscard]] uint32_t maxSize() const;
        [[nodiscard]] uint32_t remainingCapacity() const;

        void reset();

    private:
        T*      buf = nullptr;
        uint32_t  capacity = 0;
        uint32_t  head = 0;
        uint32_t  tail = 0;
        bool    full = false;
    };

    // Circular buffer (overwrites oldest data when full)
    template<typename T>
    class CircularBuffer {
    public:
        explicit CircularBuffer(etl::span<T> buffer);
        CircularBuffer() = default;

        T& getFront();
        T& getBack();

        void push(const T& item);
        void push(T&& item);
        void pop();

        [[nodiscard]] bool isEmpty() const;
        [[nodiscard]] bool isFull() const;
        [[nodiscard]] uint32_t currentSize() const;
        [[nodiscard]] uint32_t maxSize() const;
        [[nodiscard]] uint32_t remainingCapacity() const;

        void reset();

    private:
        T*      buf = nullptr;
        uint32_t  capacity = 0;
        uint32_t  head = 0;
        uint32_t  tail = 0;
        bool    full = false;
    };

    // Stable pool
    template<typename T>
    class UnorderedPool {
    public:
        explicit UnorderedPool(etl::span<T> buffer, etl::span<uint32_t> freeIndices);
        UnorderedPool() = default;

        T* push(const T& item);
        T* push(T&& item);

        bool erase(T* item);

        [[nodiscard]] bool   isEmpty()     const;
        [[nodiscard]] bool   isFull()      const;
        [[nodiscard]] uint32_t currentSize() const;
        [[nodiscard]] uint32_t maxSize()     const;
        [[nodiscard]] uint32_t remainingCapacity() const;

        void reset();

    private:
        T*      buf = nullptr;
        uint32_t  capacity = 0;
        uint32_t  sz = 0;

        // Free list as array of indices
        uint32_t* freeIndices = nullptr;
        uint32_t  freeCount = 0;
    };

} // CCSDSDataLinkLayer