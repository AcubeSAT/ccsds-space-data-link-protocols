#include "FreeRTOSTasks/DummyTask.h"
#include "ChannelPool.hpp"
#include <iostream>

void DummyTask::execute() {
    auto &vchannel = ServicePool.virtualChannel;

	//std::cout << "Yo" << std::endl;
}