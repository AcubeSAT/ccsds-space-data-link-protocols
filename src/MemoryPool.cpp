#include <cstring>
#include "MemoryPool.hpp"
#include "Logger.hpp"
#include "Alert.hpp"

namespace CCSDSDataLinkLayer {
    template<std::size_t T>
    uint8_t *MemoryPool<T>::allocateBlock(const uint16_t blockLength, const uint8_t *packetSource) {
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

    template<std::size_t T>
    bool MemoryPool<T>::deleteBlock(const uint8_t *blockStart, uint16_t blockLength) {
        int32_t indexInMemory = blockStart - &memory[0];
        if (indexInMemory >= 0 && indexInMemory + blockLength < memorySize) {
            usedMemory.erase(indexInMemory);
            return true;
        }
        LOG<Logger::error>() << "Packet not found, index is out of bounds";
        return false;
    }

    template<std::size_t T>
    std::pair<uint16_t, MasterChannelAlert> MemoryPool<T>::findFit(const uint16_t packetLength) {
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

    template<std::size_t T>
    uint8_t *MemoryPool<T>::getMemory() {
        return &memory[0];
    }
} // namespace CCSDSDataLinkLayer
