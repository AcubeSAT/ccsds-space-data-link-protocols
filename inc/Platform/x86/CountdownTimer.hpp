#pragma once

#include <chrono>

/**
 * A countdown timer (for x86) that has a running/not running state, with multiple instances being
 * able to run concurrently. Intended for use within FOP-1.
 */
class CountdownTimer {
private:
    uint16_t initialTime; // Initial time in milliseconds
    std::chrono::steady_clock::time_point startTime; // Start time point
    bool running; // Timer running state

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
    bool getRunning() const {
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
};