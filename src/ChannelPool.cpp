#include "ChannelPool.hpp"

ChannelPool::ChannelPool(): physicalChannel(1024, true, 12, 1024, 220000, 20),
      masterChannel(), serviceChannel(masterChannel,physicalChannel){

	mapChannels = {};

	masterChannel.addVC(0, 128, true, 3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED, 255, 20, 20,
	                    10);

	masterChannel.addVC(1, 128, false, 3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED, 255, 20, 20,
	                    10);

	masterChannel.addVC(2, 128, false, 3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED, 255, 3, 3, 10);
}





