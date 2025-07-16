/**
 * @file CountdownTimer.hpp
 *
 * @brief A countdown timer (for x86) that has a running/not running state, with multiple instances being
 * able to run concurrently. Intended for use within COP-1.
 *
 * @note This implementation works for x86 architecture. The user must provide its own implementation depending
 *       on the system's architecture and Operating System
 */

#pragma once
#include <chrono>

namespace CCSDSDataLinkLayer {
/**
 *
 */
    class CountdownTimer {

    public:
        CountdownTimer() : initialTime(0), running(false) {}

        /**
         *  Start the timer with a given initial time in milliseconds
         */
        void startTimer(uint16_t initialTimeMs) {
            initialTime = initialTimeMs;
            startTime = std::chrono::steady_clock::now();
            running = true;
        }

        /**
         * Stop the timer
         */
        void stopTimer() {
            running = false;
        }

        /**
         * Get running state
         */
        [[nodiscard]] bool getRunning() const {
            return running;
        }

        /**
         * Get the remaining time in milliseconds. Returns 0 if the timer has already expired,
         * or is not running.
         */
        uint16_t getRemainingTime() {
            if (!running) {
                return 0;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime
            ).count();

            // Calculate the remaining time, capped at 0
            return (elapsed >= initialTime) ? 0 : static_cast<uint16_t>(initialTime - elapsed);
        }

    private:
        uint16_t initialTime; // Initial time in milliseconds
        std::chrono::steady_clock::time_point startTime; // Start time point
        bool running; // Timer running state
    };
} // namespace CCSDSDataLinkLayer