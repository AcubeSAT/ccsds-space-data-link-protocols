#pragma once

#include <cstdint>
#include <bitset>
#include "etl/map.h"

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
	 * @var Maximum number of packets that can be allocated to the memory buffer
	 */
	static constexpr uint16_t maxAllocatedPackets = MaxAllocatedPackets;
	/**
	 * @var An array that allocates statically memory to be used for the transferFrameData data
	 */
	uint8_t memory[memorySize];

	/**
	 * @var Keep track of currently used slots. It uses an ordered map to keep track of the beginning position of each
	 * stored transferFrameData mapped to the transferFrameData's length. We used this instead of an interval tree since we are assuming
	 * non-overlapping intervals
	 */
	etl::map<uint16_t, uint16_t, maxAllocatedPackets> usedMemory;

public:
	MemoryPool() = default;

	/**
	 * This method finds the head of a contiguous block in the memory pool of a given size
	 * @param packetLength length of the data (bytes)
	 * @return A `MasterChannelAlert` is raised if there was not enough space for the data, else returns the index of
	 * the first memory where the data will be stored.
	 */
	std::pair<uint16_t, MasterChannelAlert> findFit(uint16_t packetLength);

	/**
	 * Method that copies the transferFrameData data to the first contiguous block of memory of the memory pool.
	 * Calls the `findFit` method in order to find the index of the array that is first available.
	 * @param packet pointer to the transferFrameData data.
	 * @param packetLength the length of the transferFrameData data.
	 * @return `uint8_t` pointer to the transferFrameData data in the memory pool or `nullptr` if transferFrameData could not be allocated.
	 */
	uint8_t* allocatePacket(uint8_t* packet, uint16_t packetLength);

	/**
	 * This method is called when we want to delete the data of a transferFrameData.
	 * @param packet pointer to the transferFrameData data in the pool.
	 * @param packetLength length of the data.
	 * @return true if the delete was successful and false if the transferFrameData was not found.
	 */
	bool deletePacket(uint8_t* packet, uint16_t packetLength);

	/**
	 * @return pointer to the array that stores the data.
	 */
	uint8_t* getMemory();

	/**
	 * @return the bitset that shows if each memory slot is used.
	 */
	etl::map<uint16_t, uint16_t, maxAllocatedPackets>& getUsedMemory() {
		return usedMemory;
	}
};
