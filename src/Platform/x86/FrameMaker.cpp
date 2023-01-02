#include "FrameMaker.hpp"
#include "stdlib.h"

FrameMaker::FrameMaker(ServiceChannel* servChannel){
 	serviceChannel = servChannel;
}

void FrameMaker::packetFeeder(uint8_t* packet, uint16_t packetLength,uint8_t gvid) {
	serviceChannel->storePacketTm(packet,packetLength,gvid);
	numberOfPackets++;
	frameLength += packetLength;
}


std::pair<uint8_t*, ServiceChannelNotification> FrameMaker::transmitChain(uint16_t maximumDataLength, uint8_t gcvid) {
	ServiceChannelNotification ser;
	std::pair<uint8_t*, ServiceChannelNotification> returnFrame;
	ser = serviceChannel->vcGenerationService(maximumDataLength, gcvid);
	if(ser != NO_SERVICE_EVENT){
		returnFrame.first = frame;
		returnFrame.second = ser;
		return returnFrame;
	}
	ser = serviceChannel->mcGenerationTMRequest();
	if(ser != NO_SERVICE_EVENT){
		returnFrame.first = frame;
		returnFrame.second = ser;
		return returnFrame;
	}
	ser = serviceChannel->allFramesGenerationTMRequest(frame,TmTransferFrameSize);
	if(ser != NO_SERVICE_EVENT){
		returnFrame.first = frame;
		returnFrame.second = ser;
		return returnFrame;
	}
	sentFrameLegth = frameLength;
	sentFrameNumberOfPackets = numberOfPackets;
	frameLength = 6;
	numberOfPackets = 0;
	returnFrame.first = frame;
	returnFrame.second = ser;
	return returnFrame;
}

uint16_t FrameMaker::getFrameLength() {
	return sentFrameLegth;
}

uint8_t FrameMaker::getNumberOfPackets() {
	return sentFrameNumberOfPackets;
}