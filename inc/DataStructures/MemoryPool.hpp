/**
 * @file MemoryPool.hpp
 */

#pragma once

#include <cstdint>
#include "etl/map.h"
#include "Alert.hpp"
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
		std::pair<uint16_t, MasterChannelAlert> findFit(const uint16_t packetLength) {
			std::pair<uint16_t, MasterChannelAlert> fit;
			fit.second = MasterChannelAlert::NO_MC_ALERT;

			if (packetLength > memorySize) {
				fit.second = MasterChannelAlert::NOT_ENOUGH_SPACE_IN_MEMORY_POOL;
				return fit;
			}

			const etl::imap<uint16_t, uint16_t>::iterator iteratorBegin = usedMemory.begin();
			const etl::imap<uint16_t, uint16_t>::iterator iteratorEnd = --usedMemory.end();

			// Check whether list is empty or the packet can fit in the beginning
			if (usedMemory.empty() || (iteratorBegin->first >= packetLength)) {
				fit.first = 0;
				return fit;
			}

			uint16_t gapSize;

			etl::imap<uint16_t, uint16_t>::iterator mapIterator = iteratorBegin;

			for (mapIterator; mapIterator != iteratorEnd; ++mapIterator) {
				gapSize = etl::next(mapIterator)->first - (mapIterator->first + mapIterator->second);
				if (gapSize >= packetLength) {
					fit.first = mapIterator->first + mapIterator->second;
					return fit;
				}
			}

			gapSize = memorySize - (iteratorEnd->first + iteratorEnd->second);

			// Check whether list is empty or the packet can fit in the end
			if (gapSize >= packetLength) {
				fit.first = iteratorEnd->first + iteratorEnd->second;
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
		uint8_t* allocateBlock(const uint16_t blockLength, const uint8_t *packetSource) {
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
         * @param blockLength length of the data.
         * @return true if the deletion was successful and false if the block was not found.
         */
		bool deleteBlock(const uint8_t *blockStart, uint16_t blockLength) {
			int32_t indexInMemory = blockStart - &memory[0];
			if (indexInMemory >= 0 && indexInMemory + blockLength < memorySize) {
				usedMemory.erase(indexInMemory);
				return true;
			}
			LOG<Logger::error>() << "Packet not found, index is out of bounds";
			return false;
		}

        /**
         * @return Pointer to the array that stores the data.
         */
		uint8_t* getMemory() {
			return &memory[0];
		}

        /**
         * @return The map that shows the allocated memory block positions and their lengths.
         */
        etl::map<uint16_t, uint16_t, MaxAllocatedBlocks> &getUsedMemory() {
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