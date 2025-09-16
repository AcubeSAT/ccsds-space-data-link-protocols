#include "MemoryPool.hpp"
#include "catch2/catch_all.hpp"


using namespace CCSDSDataLinkLayer;

TEST_CASE("Memory Pool Operations", "[Data Structures]") {
    const uint16_t poolSizeBytes = 5000;
    const uint16_t maxNumBlocks = 3;
    auto testMemPool = MemoryPool<poolSizeBytes, maxNumBlocks>();

    uint8_t* memoryArray = testMemPool.getMemory(); // For inspecting underlying memory

    REQUIRE(testMemPool.findFit(1000).second == MasterChannelAlert::NO_MC_ALERT);
    REQUIRE(testMemPool.findFit(5001).second == MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);

    REQUIRE(testMemPool.allocateBlock(1000, nullptr) == memoryArray);
    REQUIRE(testMemPool.allocateBlock(2000, nullptr) == memoryArray + 1000);
    REQUIRE(testMemPool.allocateBlock(2000, nullptr) == memoryArray + 3000);

    REQUIRE(testMemPool.findFit(1).second == MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
    REQUIRE(testMemPool.allocateBlock(1, nullptr) == nullptr);

    // Try deleting a block that does not exist (invalid block start)
    REQUIRE(testMemPool.deleteBlock(memoryArray + 30) == false);

    // Remove middle, last blocks and allocate 1000 bytes.
    REQUIRE(testMemPool.deleteBlock(memoryArray + 1000));
    REQUIRE(testMemPool.deleteBlock(memoryArray + 3000));
    REQUIRE(testMemPool.findFit(1000).second == MasterChannelAlert::NO_MC_ALERT);
    REQUIRE(testMemPool.allocateBlock(1000, nullptr) == memoryArray + 1000);

    // Try to allocate 3001 bytes (should not work, since 3000 bytes are remaining)
    REQUIRE(testMemPool.findFit(3001).second == MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL);
    REQUIRE(testMemPool.allocateBlock(3001, nullptr) == nullptr);

    // Now try to allocate a block while also passing data
    uint8_t dummyData[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    REQUIRE(testMemPool.findFit(10).second == MasterChannelAlert::NO_MC_ALERT);
    REQUIRE(testMemPool.allocateBlock(10, dummyData) == memoryArray + 2000);
    for (uint8_t i = 0; i < 10; i++) {
        CHECK(*(memoryArray + 2000 + i) == dummyData[i]);
    }
}