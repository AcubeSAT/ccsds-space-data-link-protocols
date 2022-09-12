#include "catch2/catch.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSServiceChannel.hpp"
#include "CLCW.hpp"
#include "stdlib.h"

ServiceChannelNotification randomServiceCallTx(uint16_t frameLength, uint8_t* frame, ServiceChannel* serviceChannel,
                                               etl::queue<uint16_t, PacketBufferTmSize>* packetsVC,
                                               etl::queue<uint16_t, PacketBufferTmSize>* packetsVCLength,
                                               etl::queue<uint16_t, PacketBufferTmSize>* framesSent,
                                               etl::queue<uint16_t, PacketBufferTmSize>* framesSentLength,
                                               uint8_t sentFrame[TmTransferFrameSize], uint8_t gcvid) {
	uint8_t randomServicePicker = rand() % 4;
	ServiceChannelNotification ser2;
	if (randomServicePicker == 0) {
		printf("Push frame VC%d - length(%d): \n", gcvid, frameLength);

		for (uint16_t i = 0; i < frameLength; i++) {
			printf("%d ", frame[i]);
		}
		printf("\n");

		serviceChannel->storePacketTm(frame, frameLength, gcvid);
		packetsVCLength->push(frameLength);
		for (int j = 0; j < frameLength; j++) {
			packetsVC->push(frame[j]);
		}
		printf("Buffer%d: ", gcvid);
		for (size_t i = 0; i < packetsVC->size(); i++) {
			printf("%d ", packetsVC->front());
			packetsVC->push(packetsVC->front());
			packetsVC->pop();
		}
		printf("\n");
		return ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
	} else if (randomServicePicker == 1) {
		serviceChannel->vcGenerationService(10, gcvid);
		return ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
	} else if (randomServicePicker == 2) {
		serviceChannel->mcGenerationTMRequest();
		return ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
		;
	} else if (randomServicePicker == 3) {
		ser2 = serviceChannel->allFramesGenerationTMRequest(sentFrame, TmTransferFrameSize);

		if (ser2 == ServiceChannelNotification::NO_SERVICE_EVENT) {
			framesSentLength->push(TmTransferFrameSize);
			for (uint8_t j = 0; j < TmTransferFrameSize; j++) {
				framesSent->push(sentFrame[j]);
			}
		}
		return ser2;
	}
}

ServiceChannelNotification randomServiceCallRx(bool* flag, uint8_t* sentPacket, uint8_t* sentPacketLength,
                                               ServiceChannel* serviceChannel,
                                               etl::queue<uint16_t, PacketBufferTmSize>* packetsVC,
                                               etl::queue<uint16_t, PacketBufferTmSize>* packetsVCLength,
                                               etl::queue<uint16_t, PacketBufferTmSize>* framesSent,
                                               etl::queue<uint16_t, PacketBufferTmSize>* framesSentLength,
                                               uint8_t receivedPacket[TmTransferFrameSize], uint8_t vid) {
	uint8_t randomServicePicker = rand() % 2;
	ServiceChannelNotification ser;
	if (randomServicePicker == 0) {
		if (!framesSentLength->empty()) {
			*sentPacketLength = framesSentLength->front();
			framesSentLength->pop();
			for (uint16_t t = 0; t < *sentPacketLength; t++) {
				sentPacket[t] = framesSent->front();
				framesSent->pop();
			}
			serviceChannel->allFramesReceptionTMRequest(sentPacket, TmTransferFrameSize);
		}

	} else if (randomServicePicker == 1) {
		ser = serviceChannel->packetExtractionTM(vid, receivedPacket);
		if (ser == ServiceChannelNotification::NO_SERVICE_EVENT) {
			if (!packetsVCLength->empty()) {
				*flag = true;
				*sentPacketLength = packetsVCLength->front();
				packetsVCLength->pop();
				for (uint16_t t = 0; t < *sentPacketLength; t++) {
					sentPacket[t] = packetsVC->front();
					packetsVC->pop();
				}
			}
			printf("Popping frame VC%d - length(%d): \n", vid, *sentPacketLength);
			printf("Buffer%d: ", vid);
			for (size_t i = 0; i < packetsVC->size(); i++) {
				printf("%d ", packetsVC->front());
				packetsVC->push(packetsVC->front());
				packetsVC->pop();
			}
			printf("\n");
		}
	}
}

TEST_CASE("TM Tx and Rx Testing") {
	PhysicalChannel physicalChannel = PhysicalChannel(1024, true, 12, 1024, 220000, 20);

	etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels = {};

	MasterChannel masterChannel = MasterChannel();

	masterChannel.addVC(0, 128, true, 3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED, 255, 20, 20,
	                    10);

	masterChannel.addVC(1, 128, false, 3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED, 255, 20, 20,
	                    10);

	masterChannel.addVC(2, 128, false, 3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED, 255, 3, 3, 10);

	ServiceChannel serviceChannel = ServiceChannel(masterChannel, physicalChannel);
	uint8_t sentFrame[TmTransferFrameSize] = {0};
	uint8_t receivedPacket[TmTransferFrameSize] = {0};
	uint8_t sentPacket[TmTransferFrameSize] = {0};
	etl::queue<uint16_t, PacketBufferTmSize> packetsVC0;
	etl::queue<uint16_t, PacketBufferTmSize> packetsVC1;
	etl::queue<uint16_t, PacketBufferTmSize> packetsVC2;
	etl::queue<uint16_t, PacketBufferTmSize> packetsVC0Length;
	etl::queue<uint16_t, PacketBufferTmSize> packetsVC1Length;
	etl::queue<uint16_t, PacketBufferTmSize> packetsVC2Length;
	etl::queue<uint16_t, PacketBufferTmSize> framesSent;
	etl::queue<uint16_t, PacketBufferTmSize> framesSentLength;
	uint8_t maximumPacketLength = 10;
	uint8_t frame[TmTransferFrameSize];
	uint8_t numberOfErrors = 0;
	uint8_t sentPacketLength = 0;
	ServiceChannelNotification serviceNotification;
	bool extractedPacket = false;
	uint16_t maximumServiceCallsTx = 100;
	uint16_t maximumServiceCallsRx = 50;
	uint16_t frameLength = maximumPacketLength;
	uint8_t virtualChannelPicker;
	uint8_t VC1UpperBoundProbability = 50;
	uint8_t VC2UpperBoundProbability = 60;
	uint8_t VC2LowerBoundProbability = 50;
	uint8_t probabilityMaximumNumber = 100;

	for (uint16_t i = 0; i < maximumServiceCallsTx; i++) {
		for (uint16_t j = 0; j < frameLength; j++) {
			frame[j] = i;
		}
		virtualChannelPicker = rand() % probabilityMaximumNumber;
		if (virtualChannelPicker <= VC1UpperBoundProbability) {
			serviceNotification = randomServiceCallTx(frameLength, frame, &serviceChannel, &packetsVC0,
			                                          &packetsVC0Length, &framesSent, &framesSentLength, sentFrame, 0);
		} else if (virtualChannelPicker > VC2LowerBoundProbability && virtualChannelPicker < VC2UpperBoundProbability) {
			serviceNotification = randomServiceCallTx(frameLength, frame, &serviceChannel, &packetsVC1,
			                                          &packetsVC1Length, &framesSent, &framesSentLength, sentFrame, 1);
		} else {
			serviceNotification = randomServiceCallTx(frameLength, frame, &serviceChannel, &packetsVC2,
			                                          &packetsVC2Length, &framesSent, &framesSentLength, sentFrame, 2);
		}

		if (serviceNotification == ServiceChannelNotification::NO_SERVICE_EVENT) {
			for (uint16_t l = 0; l < maximumServiceCallsRx; l++) {
				uint8_t virtualChannelPicker2 = rand() % probabilityMaximumNumber;
				if (virtualChannelPicker2 <= VC1UpperBoundProbability) {
					randomServiceCallRx(&extractedPacket, sentPacket, &sentPacketLength, &serviceChannel, &packetsVC0,
					                    &packetsVC0Length, &framesSent, &framesSentLength, receivedPacket, 0);
				} else if (virtualChannelPicker2 > VC2LowerBoundProbability &&
				           virtualChannelPicker2 < VC2UpperBoundProbability) {
					randomServiceCallRx(&extractedPacket, sentPacket, &sentPacketLength, &serviceChannel, &packetsVC1,
					                    &packetsVC1Length, &framesSent, &framesSentLength, receivedPacket, 1);
				} else {
					randomServiceCallRx(&extractedPacket, sentPacket, &sentPacketLength, &serviceChannel, &packetsVC2,
					                    &packetsVC2Length, &framesSent, &framesSentLength, receivedPacket, 2);
				}
				if (extractedPacket) {
					printf("\n -------------------------------------\n");
					printf("Packet temp: ");

					for (uint8_t k = 0; k < sentPacketLength; k++) {
						printf("%d ", sentPacket[k]);
						printf("%d\n", receivedPacket[k]);
						if (sentPacket[k] != receivedPacket[k]) {
							numberOfErrors++;
							printf("FAILURE :");
						}
					}

					printf("\n");
					printf("Packet dest: ");

					for (uint8_t k = 0; k < sentPacketLength; k++) {
						printf("%d ", receivedPacket[k]);
					}

					printf("\n");

					extractedPacket = false;
				}
			}
		}
	}
	CHECK(numberOfErrors == 0);
}
