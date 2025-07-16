#include "ExternalContainers.hpp"

namespace CCSDSDataLinkLayer {
    // Queue
    template<typename T>
    Queue<T>::Queue(etl::span<T> buffer)
        : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {
    }

    template<typename T>
    T &Queue<T>::getFront() { return buf[tail]; }

    template<typename T>
    T &Queue<T>::getBack() { return buf[(head + capacity - 1) % capacity]; }

    template<typename T>
    void Queue<T>::push(const T &item) {
        buf[head] = item;
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void Queue<T>::push(T &&item) {
        buf[head] = std::move(item);
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void Queue<T>::pop() {
        full = false;
        tail = (tail + 1) % capacity;
    }

    template<typename T>
    bool Queue<T>::isEmpty() const { return (!full && head == tail); }

    template<typename T>
    bool Queue<T>::isFull() const { return full; }

    template<typename T>
    uint32_t Queue<T>::currentSize() const {
        if (full) return capacity;
        if (head >= tail) return head - tail;
        return capacity + head - tail;
    }

    template<typename T>
    uint32_t Queue<T>::maxSize() const { return capacity; }

    template<typename T>
    uint32_t Queue<T>::remainingCapacity() const { return maxSize() - currentSize(); }

    template<typename T>
    void Queue<T>::reset() {
        head = tail = 0;
        full = false;
    }

    // Dequeue
    template<typename T>
    Dequeue<T>::Dequeue(etl::span<T> buffer)
        : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {
    }

    template<typename T>
    T &Dequeue<T>::getFront() { return buf[tail]; }

    template<typename T>
    T &Dequeue<T>::getBack() {
        uint32_t idx = (head + capacity - 1) % capacity;
        return buf[idx];
    }

    template<typename T>
    void Dequeue<T>::pushBack(const T &item) {
        buf[head] = item;
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void Dequeue<T>::pushBack(T &&item) {
        buf[head] = std::move(item);
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void Dequeue<T>::pushFront(const T &item) {
        tail = (tail + capacity - 1) % capacity;
        buf[tail] = item;
        full = (head == tail);
    }

    template<typename T>
    void Dequeue<T>::pushFront(T &&item) {
        tail = (tail + capacity - 1) % capacity;
        buf[tail] = std::move(item);
        full = (head == tail);
    }

    template<typename T>
    void Dequeue<T>::popFront() {
        full = false;
        tail = (tail + 1) % capacity;
    }

    template<typename T>
    void Dequeue<T>::popBack() {
        full = false;
        head = (head + capacity - 1) % capacity;
    }

    template<typename T>
    bool Dequeue<T>::isEmpty() const { return (!full && head == tail); }

    template<typename T>
    bool Dequeue<T>::isFull() const { return full; }

    template<typename T>
    uint32_t Dequeue<T>::currentSize() const {
        if (full) return capacity;
        if (head >= tail) return head - tail;
        return capacity + head - tail;
    }

    template<typename T>
    uint32_t Dequeue<T>::maxSize() const { return capacity; }

    template<typename T>
    uint32_t Dequeue<T>::remainingCapacity() const { return maxSize() - currentSize(); }

    template<typename T>
    void Dequeue<T>::reset() {
        head = tail = 0;
        full = false;
    }

    // CircularBuffer
    template<typename T>
    CircularBuffer<T>::CircularBuffer(etl::span<T> buffer)
        : buf(buffer.data()), capacity(buffer.size()), head(0), tail(0), full(false) {
    }

    template<typename T>
    T &CircularBuffer<T>::getFront() { return buf[tail]; }

    template<typename T>
    T &CircularBuffer<T>::getBack() {
        uint32_t idx = (head + capacity - 1) % capacity;
        return buf[idx];
    }

    template<typename T>
    void CircularBuffer<T>::push(const T &item) {
        buf[head] = item;
        if (full) tail = (tail + 1) % capacity;
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void CircularBuffer<T>::push(T &&item) {
        buf[head] = std::move(item);
        if (full) tail = (tail + 1) % capacity;
        head = (head + 1) % capacity;
        full = (head == tail);
    }

    template<typename T>
    void CircularBuffer<T>::pop() {
        full = false;
        tail = (tail + 1) % capacity;
    }

    template<typename T>
    bool CircularBuffer<T>::isEmpty() const { return (!full && head == tail); }

    template<typename T>
    bool CircularBuffer<T>::isFull() const { return full; }

    template<typename T>
    uint32_t CircularBuffer<T>::currentSize() const {
        if (full) return capacity;
        if (head >= tail) return head - tail;
        return capacity + head - tail;
    }

    template<typename T>
    uint32_t CircularBuffer<T>::maxSize() const { return capacity; }

    template<typename T>
    uint32_t CircularBuffer<T>::remainingCapacity() const { return maxSize() - currentSize(); }

    template<typename T>
    void CircularBuffer<T>::reset() {
        head = tail = 0;
        full = false;
    }

    template<typename T>
    UnorderedPool<T>::UnorderedPool(etl::span<T> buffer, etl::span<uint32_t> freeIndices)
        : buf(buffer.data()), capacity(buffer.size()), sz(0),
          freeIndices(freeIndices.data()), freeCount(buffer.size()) {

        // Initialize free indices stack
        for (uint32_t i = 0; i < capacity; ++i) {
            this->freeIndices[i] = i;
        }
    }

    template<typename T>
    T* UnorderedPool<T>::push(const T& item) {
        // Pop index from free stack
        uint32_t index = freeIndices[--freeCount];

        // Construct object at that index
        buf[index] = item;

        ++sz;
        return &buf[index];
    }

    template<typename T>
    T* UnorderedPool<T>::push(T&& item) {
        // Pop index from free stack
        uint32_t index = freeIndices[--freeCount];

        // Construct object at that index
        buf[index] = std::move(item);

        ++sz;
        return &buf[index];
    }

    template<typename T>
    bool UnorderedPool<T>::erase(T* item) {
        if (!item || item < buf || item >= buf + capacity) {
            return false;  // Invalid pointer
        }

        // Calculate index
        uint32_t index = item - buf;

        // Push index back to free stack
        freeIndices[freeCount++] = index;

        --sz;
        return true;
    }

    template<typename T>
    bool UnorderedPool<T>::isEmpty() const {
        return sz == 0;
    }

    template<typename T>
    bool UnorderedPool<T>::isFull() const {
        return freeCount == 0;
    }

    template<typename T>
    uint32_t UnorderedPool<T>::currentSize() const {
        return sz;
    }

    template<typename T>
    uint32_t UnorderedPool<T>::maxSize() const {
        return capacity;
    }

    template<typename T>
    uint32_t UnorderedPool<T>::remainingCapacity() const {
        return maxSize() - currentSize();
    }

    template<typename T>
    void UnorderedPool<T>::reset() {
        sz = 0;
        freeCount = capacity;

        // Reinitialize free indices
        for (uint32_t i = 0; i < capacity; ++i) {
            freeIndices[i] = i;
        }
    }
} // CCSDSDataLinkLayer
