/**
 * @file MemoryPool.hpp
 */

#pragma once

#include <cstdint>
#include "etl/map.h"
#include "Alert.hpp"
#include "CCSDSDefinitionsAndUtilities.hpp"

namespace CCSDSDataLinkLayer {
/**
 * @class MemoryPool This class defines a custom memory pool which is a statically allocated block of memory.
 * This helps better keep track of data and reduce the memory needed for storing them.
 */

	template <std::size_t T>
    class MemoryPool {
    private:
        /**
         * @var The size of the block of memory in bytes
         */
        static constexpr uint16_t memorySize = T;

        /**
         * @var Maximum number of packets that can be allocated to the memory buffer
         */
        static constexpr uint16_t maxAllocatedPackets = DefsAndUtils::MaxAllocatedFramesInMemoryPool;
        /**
         * @var An array that allocates statically memory to be used for the packet data
         */
        uint8_t memory[T];

        /**
         * @var Keep track of currently used slots. It uses an ordered map to keep track of the beginning position of each
         * stored packet mapped to the packet's length. We used this instead of an interval tree since we are assuming
         * non-overlapping intervals
         */
        etl::map<uint16_t, uint16_t, maxAllocatedPackets> usedMemory;

    public:
        MemoryPool() = default;

        /**
         * @brief This method finds the head of a contiguous block in the memory pool of a given size
         * @param packetLength length of the data (bytes)
         * @return A `MasterChannelAlert` is raised if there was not enough space for the block, else returns the index of
         * the first memory where the data will be stored.
         */
        std::pair<uint16_t, MasterChannelAlert> findFit(uint16_t packetLength);

        /**
         * @brief Method that allocates the first contiguous block of memory of the memory pool and optionally
         *        copies packet data to that block.
         * @details Calls the `findFit` method in order to find the index of the array that is first available.
         * @param blockLength The length of the packet block.
	     * @param packetSource Pointer to the packet data. In order for a copy to not take place, this value should be nullptr.
         * @return A `uint8_t` pointer to the block start in the memory pool or `nullptr` if no such block could be
         *          allocated.
         */
        uint8_t* allocateBlock(uint16_t blockLength, const uint8_t *packetSource = nullptr);

        /**
         * @brief This method is called when we want to deallocate a block and delete the data of a packet.
         * @param blockStart pointer to the packet data in the pool.
         * @param blockLength length of the data.
         * @return true if the deletion was successful and false if the block was not found.
         */
        bool deleteBlock(const uint8_t *blockStart, uint16_t blockLength);

        /**
         * @return Pointer to the array that stores the data.
         */
        uint8_t* getMemory();

        /**
         * @return The map that shows the allocated memory block positions and their lengths.
         */
        etl::map<uint16_t, uint16_t, maxAllocatedPackets> &getUsedMemory() {
            return usedMemory;
        }
    };
} // namespace CCSDSDataLinkLayer