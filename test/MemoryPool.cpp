//#include <catch2/catch.hpp>
#include <catch2/catch_test_macros.hpp>
#include <MemoryPool.hpp>

TEST_CASE("Packet Insertions") {
	// Initialize packets
	uint8_t data[5] = {1, 2, 3, 4, 5};
	uint8_t data2[5] = {1, 2, 3, 4, 5};
	uint8_t data3[1] = {100};
	uint8_t data4[1] = {65};
	uint8_t data5[1] = {71};

	// Insertions
	SECTION("Packet insertion with empty memory") {
		// Initialize memory pool
		MemoryPool pool = MemoryPool();

		uint8_t* packet = pool.allocatePacket(data, 5);
		CHECK(pool.getUsedMemory()[0] == 5);
		for (unsigned int i = 0; i < 5; i++) {
			CHECK(pool.getMemory()[i] == data[i]);
		}
	}
	SECTION("Bigger packet than empty space") {
		// Initialize memory pool
		MemoryPool pool = MemoryPool();

		uint8_t* packet = pool.allocatePacket(data, 5);
		CHECK(packet != nullptr);
		// TODO: When the buffers are templated change this test
		uint8_t* packet2 = pool.allocatePacket(data2, 5);
		CHECK(packet2 != nullptr);
		CHECK(pool.getUsedMemory()[0] == 5);
		CHECK(pool.getUsedMemory()[5] == 5);
	}
	SECTION("Packet insertions with some space left") {
		// Initialize memory pool
		MemoryPool pool = MemoryPool();

		uint8_t* packet = pool.allocatePacket(data, 5);
		uint8_t* packet3 = pool.allocatePacket(data3, 1);
		CHECK(packet3 != nullptr);
		CHECK(pool.getUsedMemory()[0] == 5);
		CHECK(pool.getUsedMemory()[5] == 1);
		CHECK(pool.getMemory()[5] == 100);

		uint8_t* packet4 = pool.allocatePacket(data4, 1);
		CHECK(packet4 != nullptr);
		CHECK(pool.getUsedMemory()[6] == 1);
		CHECK(pool.getMemory()[6] == data4[0]);
	}
	SECTION("Packet length equals space left") {
		// Initialize memory pool
		MemoryPool pool = MemoryPool();

		uint8_t* packet = pool.allocatePacket(data, 5);
		uint8_t* packet3 = pool.allocatePacket(data3, 1);
		uint8_t* packet4 = pool.allocatePacket(data4, 1);
		uint8_t* packet5 = pool.allocatePacket(data5, 1);
		CHECK(pool.getUsedMemory()[7] == true);
		CHECK(pool.getMemory()[7] == data5[0]);
	}
}

TEST_CASE("Packet deletions") {
	// Initialize memory pool
	MemoryPool pool = MemoryPool();

	// Initialize packets
	uint8_t data[5] = {1, 2, 3, 4, 5};
	uint8_t data2[5] = {1, 2, 3, 4, 5};
	uint8_t data3[1] = {100};
	uint8_t data4[1] = {65};
	uint8_t data5[1] = {71};

	// Allocate the packets as before
	uint8_t* packet = pool.allocatePacket(data, 5);
	uint8_t* packet2 = pool.allocatePacket(data2, 5);
	uint8_t* packet3 = pool.allocatePacket(data3, 1);
	uint8_t* packet4 = pool.allocatePacket(data4, 1);
	uint8_t* packet5 = pool.allocatePacket(data5, 1);

	SECTION("Delete packet that has not been stored") {
		pool.deletePacket(packet2, 5);
		CHECK(pool.deletePacket(packet2, 5) == true);
		CHECK(pool.deletePacket(packet2, 5) == true);
		CHECK(pool.getUsedMemory()[5] == 0);
	}

	SECTION("Delete packet that has been stored") {
		pool.deletePacket(packet4, 1);
		CHECK(pool.deletePacket(packet4, 1) == true);
		CHECK(pool.getUsedMemory()[11] == 0);

		pool.deletePacket(packet, 5);
		CHECK(pool.deletePacket(packet, 5) == true);
		CHECK(pool.getUsedMemory()[0] == 0);
	}
}