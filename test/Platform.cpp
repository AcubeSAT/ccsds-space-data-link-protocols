#include <chrono>
#include <thread>
#include "CountdownTimer.hpp"
#include "Mutex.hpp"
#include "CcsdsDefinitions.hpp"
#include "catch2/catch_all.hpp"

using namespace CCSDSDataLinkLayer;

TEST_CASE("Countdown timer", "[Platform]") {
    auto timer = CountdownTimer();
    uint16_t initialTimeMs = 500;
    auto approxEqual = [](int a, int b, int deviation) -> bool {return std::abs(a - b) <= deviation;};

    timer.startTimer(initialTimeMs);

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    REQUIRE(approxEqual(timer.getRemainingTime(), initialTimeMs / 2, 1));

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    REQUIRE(timer.getRemainingTime() == 0);
}

TEST_CASE("Mutex", "[Platform]") {
    auto mutex = Mutex();

    // lock mutex
    REQUIRE(mutex.tryLockFor(Defs::MutexDelayMs));

    // should be possible to lock again
    REQUIRE(!mutex.tryLockFor(Defs::MutexDelayMs));

    mutex.unlock();

    // try to lock again
    REQUIRE(mutex.tryLockFor(Defs::MutexDelayMs));
}