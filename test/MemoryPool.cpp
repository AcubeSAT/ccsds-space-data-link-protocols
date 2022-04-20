#include <catch2/catch.hpp>
#include <MemoryPool.h>

TEST_CASE("PACKET INSERTIONS AND DELETIONS"){
    //initalize memory pool
    MemoryPool* pool = new MemoryPool();

    //intialize packets
    uint8_t data[5] = {1,2,3,4,5};
    uint8_t data2[5] = {1,2,3,4,5};
    uint8_t data3[1] = {100};
    uint8_t data4[1] = {65};
    uint8_t data5[1] = {71};

    //Insertions
    uint8_t * packet = pool->allocatePacket(data,5);
    for(unsigned int i = 0; i<5 ; i++){
        CHECK(pool->getUsedMemory()[i] == true);
        CHECK(pool->getMemory()[i] == data[i]);
    }
    uint8_t * packet2 = pool->allocatePacket(data2, 5);
    for(unsigned int i = 5; i<8 ; i++){
        CHECK(pool->getUsedMemory()[i] == false);
    }
    uint8_t * packet3 = pool->allocatePacket(data3, 1);
    CHECK(pool->getUsedMemory()[5] == true);
    CHECK(pool->getMemory()[5] == data3[0]);
    uint8_t * packet4 = pool->allocatePacket(data4, 1);
    CHECK(pool->getUsedMemory()[6] == true);
    CHECK(pool->getMemory()[6] == data4[0]);
    uint8_t * packet5 = pool->allocatePacket(data5, 1);
    CHECK(pool->getUsedMemory()[7] == true);
    CHECK(pool->getMemory()[7] == data5[0]);

    //Deletions
    pool->deletePacket(packet2, 5);
    CHECK(pool->deletePacket(packet2, 5) == false);
    for(unsigned int i = 5; i<8 ; i++){
        CHECK(pool->getUsedMemory()[i] == true);
    }
    pool->deletePacket(packet4, 1);
    CHECK(pool->deletePacket(packet4, 1) == true);
    CHECK(pool->getUsedMemory()[6] == false);

    pool->deletePacket(packet, 5);
    CHECK(pool->deletePacket(packet, 5) == true);
    for(unsigned int i = 0; i<5 ; i++){
        CHECK(pool->getUsedMemory()[i] == false);
    }

}

