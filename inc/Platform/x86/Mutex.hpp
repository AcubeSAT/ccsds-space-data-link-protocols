/**
* @file Mutex.hpp
 *
 * @brief A mutex to protect different data link queues from concurrent access
 *
 * @note This implementation works for x86 architecture. The user must provide its own implementation depending
 *       on the system's architecture and Operating System
 */

#pragma once
#include <mutex>
#include <chrono>

namespace CCSDSDataLinkLayer {
    class Mutex {
    public:
        Mutex() = default;

        /**
         * @brief Attempt to lock the mutex for timeMs milliseconds. The process is blocked during that time
         * @param timeMs Maximum time to wait for the lock, in milliseconds
         * @return true if the lock was acquired within the timeout, false otherwise
         *
         * @warning To avoid deadlocks, a strict lock hierarchy is used:
         * 1. cop-1 mutexes
         * 2. map channel mutexes
         * 3. virtual channel mutexes
         * 4. master channel mutexes
         * 5. security association mutexes
         * 6. frameOctetPool mutex

         */
        bool tryLockFor(uint32_t timeMs) {
            return mutex.try_lock_for(std::chrono::milliseconds(timeMs));
        }

        /**
         * @brief Unlock the mutex
         *
         * @warning To avoid deadlocks, a strict unlocking hierarchy is used (the reverse of the locking hierarchy):
         * 1. frameOctetPool mutex
         * 2. security association mutexes
         * 3. master channel mutexes
         * 4. virtual channel mutexes
         * 5. map channel mutexes
         * 6. cop-1 mutexes
         */
        void unlock() {
            mutex.unlock();
        }

    private:
        std::timed_mutex mutex;
    };
} // namespace CCSDSDataLinkLayer
