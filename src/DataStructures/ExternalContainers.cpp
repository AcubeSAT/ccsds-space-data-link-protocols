#include "ExternalContainers.hpp"

namespace CCSDSDataLinkLayer {
    template<typename T>
    Queue<T>::Queue(etl::span<T> buffer)
        : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {}

    template<typename T>
    T& Queue<T>::getFront() {
        return buf[head];
    }

    template<typename T>
    void Queue<T>::push(const T& item) {
        buf[head] = item;
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void Queue<T>::pop(T& item) {
        item = buf[tail];
        full = false;
        tail = (tail + 1) % capacity;
    }

    template<typename T>
    bool Queue<T>::isEmpty() const {
        return (!full && head == tail);
    }

    template<typename T>
    bool Queue<T>::isFull() const {
        return full;
    }

    template<typename T>
    size_t Queue<T>::currentSize() const {
        if (full) {
            return capacity;
        }

        if (head >= tail) {
            return head - tail;
        }
        return capacity + head - tail;
    }

    template<typename T>
    size_t Queue<T>::maxSize() const {
        return capacity;
    }

    template<typename T>
    void Queue<T>::reset() {
        head = 0;
        tail = 0;
        full = false;
    }

    // Dequeue definitions

    template<typename T>
    Dequeue<T>::Dequeue(etl::span<T> buffer)
        : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {}

    template<typename T>
    T& Dequeue<T>::getFront() {
        return buf[head];
    }

    template<typename T>
    void Dequeue<T>::pushBack(const T& item) {
        buf[head] = item;
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void Dequeue<T>::pushFront(const T& item) {
        tail = (tail + capacity - 1) % capacity;
        buf[tail] = item;
        full = (head == tail);
    }

    template<typename T>
    void Dequeue<T>::popFront(T& item) {
        item = buf[tail];
        full = false;
        tail = (tail + 1) % capacity;
    }

    template<typename T>
    void Dequeue<T>::popBack(T& item) {
        head = (head + capacity - 1) % capacity;
        item = buf[head];
        full = false;
    }

    template<typename T>
    bool Dequeue<T>::isEmpty() const {
        return (!full && head == tail);
    }

    template<typename T>
    bool Dequeue<T>::isFull() const {
        return full;
    }

    template<typename T>
    size_t Dequeue<T>::currentSize() const {
        if (full) {
            return capacity;
        }
        if (head >= tail) {
            return head - tail;
        }
        return capacity + head - tail;
    }

    template<typename T>
    size_t Dequeue<T>::maxSize() const {
        return capacity;
    }

    template<typename T>
    void Dequeue<T>::reset() {
        head = 0;
        tail = 0;
        full = false;
    }

    // CircularBuffer definitions (overwrites when full)

    template<typename T>
    CircularBuffer<T>::CircularBuffer(etl::span<T> buffer)
        : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {}

    template<typename T>
    T& CircularBuffer<T>::getFront() {
        return buf[head];
    }

    template<typename T>
    void CircularBuffer<T>::push(const T& item) {
        buf[head] = item;

        if (full) {
            // Overwrite mode: advance tail when full
            tail = (tail + 1) % capacity;
        }

        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void CircularBuffer<T>::pop(T& item) {
        item = buf[tail];
        full = false;
        tail = (tail + 1) % capacity;
    }

    template<typename T>
    bool CircularBuffer<T>::isEmpty() const {
        return (!full && head == tail);
    }

    template<typename T>
    bool CircularBuffer<T>::isFull() const {
        return full;
    }

    template<typename T>
    size_t CircularBuffer<T>::currentSize() const {
        if (full) {
            return capacity;
        }
        if (head >= tail) {
            return head - tail;
        }
        return capacity + head - tail;
    }

    template<typename T>
    size_t CircularBuffer<T>::maxSize() const {
        return capacity;
    }

    template<typename T>
    void CircularBuffer<T>::reset() {
        head = 0;
        tail = 0;
        full = false;
    }

    // Unordered Pool definitions
    template<typename T>
    UnorderedPool<T>::UnorderedPool(etl::span<T> buffer)
        : buf(buffer.data()), capacity(buffer.size()), sz(0) {
    }

    template<typename T>
    void UnorderedPool<T>::push(const T &item) {
        assert(sz < capacity);
        buf[sz++] = item;
    }

    template<typename T>
    bool UnorderedPool<T>::erase(T& item) {
        T* ptr = &item;
        if (ptr < buf || ptr >= buf + sz) {
            // item not in pool
            return false;
        }
        size_t idx = static_cast<size_t>(ptr - buf);

        // Swap with last element if not already last
        if (idx + 1 != sz) {
            buf[idx] = std::move(buf[sz - 1]);
        }
        --sz;
        return true;
    }

    template<typename T>
    T &UnorderedPool<T>::getFront() {
        assert(sz > 0);
        return buf[0];
    }

    template<typename T>
    bool UnorderedPool<T>::isEmpty() const {
        return sz == 0;
    }

    template<typename T>
    bool UnorderedPool<T>::isFull() const {
        return sz == capacity;
    }

    template<typename T>
    size_t UnorderedPool<T>::currentSize() const {
        return sz;
    }

    template<typename T>
    size_t UnorderedPool<T>::maxSize() const {
        return capacity;
    }

    template<typename T>
    void UnorderedPool<T>::reset() {
        sz = 0;
    }
} // namespace CCSDSDataLinkLayer
