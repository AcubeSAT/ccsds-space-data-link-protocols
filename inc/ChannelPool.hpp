#ifndef CCSDS_TM_PACKETS_CHANNELPOOL_HPP
#define CCSDS_TM_PACKETS_CHANNELPOOL_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSServiceChannel.hpp"
#include "CLCW.hpp"
#include "FrameAcceptanceReporting.hpp"
#include "FrameOperationProcedure.hpp"
#include "MemoryPool.hpp"
#include "TransferFrame.hpp"
#include "TransferFrameTC.hpp"
#include "TransferFrameTM.hpp"

class ChannelPool {
public:
	PhysicalChannel physicalChannel;

	ServiceChannel serviceChannel;

	MasterChannel masterChannel;

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels;

	ChannelPool();
};

extern ChannelPool Channels;

#endif // CCSDS_TM_PACKETS_CHANNELPOOL_HPP
