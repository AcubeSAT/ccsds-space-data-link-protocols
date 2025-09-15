/**
 * @file MemoryPool.hpp
 */

#pragma once
#include <cstring>
#include <cstdint>
#include "etl/map.h"
#include "DataLinkNotifications.hpp"
#include "Mutex.hpp"
#include "Logger.hpp"

namespace CCSDSDataLinkLayer {
/**
 * @class MemoryPool This class defines a custom memory pool which is a statically allocated block of memory.
 * This helps better keep track of data and reduce the memory needed for storing them.
 */

	template <std::size_t SizeBytes, std::size_t MaxAllocatedBlocks>
    class MemoryPool {
    public:
        MemoryPool() = default;

		Mutex poolMutex = Mutex();

        /**
         * @brief This method finds the head of a contiguous block in the memory pool of a given size
         * @param packetLength length of the data (bytes)
         * @return A `MasterChannelAlert` is raised if there was not enough space for the block, else returns the index of
         * the first memory where the data will be stored.
         */
		[[nodiscard]] std::pair<uint16_t, MasterChannelAlert> findFit(const uint16_t packetLength) {
			std::pair<uint16_t, MasterChannelAlert> fit;
			fit.second = MasterChannelAlert::NO_MC_ALERT;

			if (packetLength > memorySize) {
				fit.second = MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL;
				return fit;
			}

			if (usedMemory.empty()) {
				fit.first = 0;
				return fit;
			}

			auto it = usedMemory.begin();
			// Check gap before the first block
			if (it->first >= packetLength) {
				fit.first = 0;
				return fit;
			}

			// Check gaps between blocks
			auto prevIt = it;
			++it;
			for (; it != usedMemory.end(); ++it) {
				uint16_t gapSize = it->first - (prevIt->first + prevIt->second);
				if (gapSize >= packetLength) {
					fit.first = prevIt->first + prevIt->second;
					return fit;
				}
				prevIt = it;
			}

			// Check gap after last block
			uint16_t gapSize = memorySize - (prevIt->first + prevIt->second);
			if (gapSize >= packetLength) {
				fit.first = prevIt->first + prevIt->second;
				return fit;
			}

			fit.second = MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL;
			return fit;
		}


        /**
         * @brief Method that allocates the first contiguous block of memory of the memory pool and optionally
         *        copies packet data to that block.
         * @details Calls the `findFit` method in order to find the index of the array that is first available.
         * @param blockLength The length of the packet block.
	     * @param packetSource Pointer to the packet data. In order for a copy to not take place, this value should be nullptr.
         * @return A `uint8_t` pointer to the block start in the memory pool or `nullptr` if no such block could be
         *          allocated.
         */
		[[nodiscard]] uint8_t* allocateBlock(const uint16_t blockLength, const uint8_t *packetSource) {
			std::pair<uint16_t, MasterChannelAlert> index = findFit(blockLength);
			uint16_t start = index.first;
			if (index.second == MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL) {
				LOG_ERROR << "There is no space in memory pool for the packet.";
				return nullptr;
			}

			if (packetSource != nullptr) {
				std::memcpy(memory + start, packetSource, blockLength * sizeof(uint8_t));
			}

			usedMemory[index.first] = blockLength;

			return memory + start;
		}

        /**
         * @brief This method is called when we want to deallocate a block and delete the data of a packet.
         * @param blockStart pointer to the packet data in the pool.
         * @return true if the deletion was successful and false if the block was not found.
         */
		bool deleteBlock(const uint8_t *blockStart) {
			int32_t indexInMemory = blockStart - &memory[0];
			auto it = usedMemory.find(indexInMemory);
			if (it == usedMemory.end()) {
				LOG<Logger::error>() << "Did not find allocated packet in this address";
				return false;
			}
			usedMemory.erase(it);
			return true;
		}

        /**
         * @return Pointer to the array that stores the data.
         */
		[[nodiscard]] uint8_t* getMemory() {
			return &memory[0];
		}

        /**
         * @return The map that shows the allocated memory block positions and their lengths.
         */
        [[nodiscard]] etl::map<uint16_t, uint16_t, MaxAllocatedBlocks> &getUsedMemory() {
            return usedMemory;
        }

	private:
	    /**
         * @var The size of the block of memory in bytes
         */
	    static constexpr uint16_t memorySize = SizeBytes;

	    /**
         * @var An array that allocates statically memory to be used for the packet data
         */
	    uint8_t memory[SizeBytes];

	    /**
         * @var Keep track of currently used slots. It uses an ordered map to keep track of the beginning position of each
         * stored packet mapped to the packet's length. We used this instead of an interval tree since we are assuming
         * non-overlapping intervals
         */
	    etl::map<uint16_t, uint16_t, MaxAllocatedBlocks> usedMemory;
    };
} // namespace CCSDSDataLinkLayer