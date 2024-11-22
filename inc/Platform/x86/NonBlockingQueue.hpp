#pragma once
#include <mutex>
#include <etl/queue.h>

/**
 * Implement a very simple, fixed sized queue that does not block a thread when a write/receive operation
 * fails. It uses an std::mutex and std::mutex::try_lock.
 */
template <typename T, uint16_t MaxSize> class NonBlockingQueue {
public:
    /**
     * Attempt to push data to the queue. Returns a boolean indicating
     * whether operation was successful or not.
     */
    bool push(T data) {
      if (nonBlockingQueueMutex.try_lock()) {
          if (!nonBlockingQueue.full()) {
              nonBlockingQueue.push(data);
              nonBlockingQueueMutex.unlock();
              return true;
          }
          else {
              nonBlockingQueueMutex.unlock();
              return false;
          }
      }
      else {
          return false;
      }
    };

    /**
     * Attempt to pop data from the queue. Returns a boolean indicating
     * whether operation was successful or not.
     */
    bool pop(T& data) {
        if (nonBlockingQueueMutex.try_lock()) {
            if (!nonBlockingQueue.empty()) {
                data = nonBlockingQueue.front();
                nonBlockingQueue.pop();
                nonBlockingQueueMutex.unlock();
                return true;
            }
            else {
                nonBlockingQueueMutex.unlock();
                return false;
            }
        }
        else {
            return false;
        }
    };

private:
    etl::queue<T, MaxSize> nonBlockingQueue;
    std::mutex nonBlockingQueueMutex;
};