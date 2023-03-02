#pragma once

#include <cstdint>
#include <bitset>
#include "etl/vector.h"
#include "Alert.hpp"
#include "CCSDS_Definitions.hpp"
/**
 * @class MemoryPool This class defines a custom memory pool which is a statically allocated block of memory.
 * This helps better keep track of data and reduce the memory needed for storing them.
 */

class MemoryPool {
private:
	/**
	 * @var The size of the block of memory in bytes
	 */
	static constexpr uint16_t memorySize = MemoryPoolMemorySize;

	/**
	 * @var An array that allocates statically memory to be used for the packet data
	 */
	uint8_t memory[memorySize];

	/**
	 * @var a bitset that shows if each memory slot is used. If bit in place "i" is False (0), memory slot "i" is empty
	 * and if bit is True (1), the corresponding memory slot is used.
	 */
	std::bitset<memorySize> usedMemory = std::bitset<memorySize>(0);

public:
	MemoryPool() = default;

	/**
	 * This method finds the head of a contiguous block in the memory pool of a given size
	 * @param packetLength length of the data (bytes)
	 * @return A `MasterChannelAlert` is raised if there was not enough space for the data, else returns the index of the first
	 * memory where the data will be stored.
	 */
	std::pair<uint16_t, MasterChannelAlert> findFit(uint16_t packetLength);

	/**
	 * Method that copies the packet data to the first contiguous block of memory of the memory pool.
	 * Calls the `findFit` method in order to find the index of the array that is first available.
	 * @param packet pointer to the packet data.
	 * @param packetLength the length of the packet data.
	 * @return `uint8_t` pointer to the packet data in the memory pool or `nullptr` if packet could not be allocated.
	 */
	uint8_t* allocatePacket(uint8_t* packet, uint16_t packetLength);

	/**
	 * This method is called when we want to delete the data of a packet.
	 * @param packet pointer to the packet data in the pool.
	 * @param packetLength length of the data.
	 * @return true if the delete was successful and false if the packet was not found.
	 */
	bool deletePacket(uint8_t* packet, uint16_t packetLength);

	/**
	 * @return pointer to the array that stores the data.
	 */
	uint8_t* getMemory();

	/**
	 * @return the bitset that shows if each memory slot is used.
	 */
	std::bitset<memorySize> &getUsedMemory(){
	    return usedMemory;
    }
};
