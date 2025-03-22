#include <cstring>
#include "MemoryPool.hpp"
#include "Logger.hpp"
#include "Alert.hpp"

namespace CCSDSDataLinkLayer {
    template<std::size_t T>
    uint8_t *MemoryPool<T>::allocatePacket(uint8_t *packet, uint16_t packetLength) {
        std::pair<uint16_t, MasterChannelAlert> index = findFit(packetLength);
        uint16_t start = index.first;
        if (index.second == MasterChannelAlert::NO_SPACE) {
            LOG_ERROR << "There is no space in memory pool for the packet.";
            return nullptr;
        }
        std::memcpy(memory + start, packet, packetLength * sizeof(uint8_t));

        usedMemory[index.first] = packetLength;

        return memory + start;
    }

    template<std::size_t T>
    bool MemoryPool<T>::deletePacket(const uint8_t *packet, uint16_t packetLength) {
        int32_t indexInMemory = packet - &memory[0];
        if (indexInMemory >= 0 && indexInMemory + packetLength < memorySize) {
            usedMemory.erase(indexInMemory);
            return true;
        }
        LOG<Logger::error>() << "Packet not found, index is out of bounds";
        return false;
    }

    template<std::size_t T>
    std::pair<uint16_t, MasterChannelAlert> MemoryPool<T>::findFit(uint16_t packetLength) {
        std::pair<uint16_t, MasterChannelAlert> fit;
        fit.second = MasterChannelAlert::NO_MC_ALERT;

        if (packetLength > memorySize) {
            fit.second = MasterChannelAlert::NO_SPACE;
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

        for (mapIterator; mapIterator != iteratorEnd; mapIterator++) {
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

        fit.second = MasterChannelAlert::NO_SPACE;
        return fit;
    }

    template<std::size_t T>
    uint8_t *MemoryPool<T>::getMemory() {
        return &memory[0];
    }
} // namespace CCSDSDataLinkLayer
