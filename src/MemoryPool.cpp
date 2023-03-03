#include "MemoryPool.hpp"
#include "cstring"
#include "Logger.hpp"
#include "Alert.hpp"
#include "CCSDS_Definitions.hpp"

uint8_t* MemoryPool::allocatePacket(uint8_t* packet, uint16_t packetLength) {
	std::pair<uint16_t, MasterChannelAlert> index = findFit(packetLength);
	uint16_t start = index.first;
	if (index.second == NO_SPACE) {
		LOG_ERROR << "There is no space in memory pool for the packet.";
		return nullptr;
	}
	std::memcpy(memory + start, packet, packetLength * sizeof(uint8_t));
	
	usedMemory[index.first] = packetLength;

	return memory + start;
}

bool MemoryPool::deletePacket(uint8_t* packet, uint16_t packetLength) {
	int32_t indexInMemory = packet - &memory[0];
	if (indexInMemory >= 0 && indexInMemory + packetLength < memorySize) {
		usedMemory.erase(indexInMemory);
		return true;
	}
	LOG<Logger::error>() << "Packet not found, index is out of bounds";
	return false;
}

std::pair<uint16_t, MasterChannelAlert> MemoryPool::findFit(uint16_t packetLength) {
	std::pair<uint16_t, MasterChannelAlert> fit;
	fit.second = MasterChannelAlert::NO_MC_ALERT;
	
	if (packetLength > memorySize){
		fit.second = NO_SPACE;
		return fit;
	}

    const etl::imap<uint16_t, uint16_t>::iterator it_begin = usedMemory.begin();
    const etl::imap<uint16_t, uint16_t>::iterator it_end = --usedMemory.end();

	// Check whether list is empty or the packet can fit in the beginning
	if (usedMemory.empty() || (it_begin->first >= packetLength)){
		usedMemory[0] = packetLength; 
		fit.first = 0;
		return fit;
	}

    uint16_t gap_size;

    etl::imap<uint16_t, uint16_t>::iterator it = it_begin;

	for (it; it != it_end; it++){
        gap_size = etl::next(it)->first - (it->first + it->second);
		if (gap_size >= packetLength){
			usedMemory[it->first + it->second] = packetLength;
			fit.first = it->first + it->second;
			return fit;
		}  		
    }

	gap_size = memorySize - (it_end->first + it_end->second);
	
	// Check whether list is empty or the packet can fit in the end
	if (gap_size >= packetLength){
		usedMemory[it_end->first + it_end->second] = packetLength;
		fit.first = it_end->first + it_end->second;
		return fit;
	}

	fit.second = NO_SPACE;
	return fit;
}

uint8_t* MemoryPool::getMemory() {
	return &memory[0];
}
