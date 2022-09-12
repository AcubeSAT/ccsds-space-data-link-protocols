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
                                               uint8_t packetDestination[TmTransferFrameSize], uint8_t gcvid) {
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
		ser2 = serviceChannel->allFramesGenerationTMRequest(packetDestination, TmTransferFrameSize);

		if (ser2 == ServiceChannelNotification::NO_SERVICE_EVENT) {
			framesSentLength->push(TmTransferFrameSize);
			for (uint8_t j = 0; j < TmTransferFrameSize; j++) {
				framesSent->push(packetDestination[j]);
			}
		}
		return ser2;
	}
}

ServiceChannelNotification randomServiceCallRx(bool* flag, uint8_t* tmpData, uint8_t* tmplength,
                                               ServiceChannel* serviceChannel,
                                               etl::queue<uint16_t, PacketBufferTmSize>* packetsVC,
                                               etl::queue<uint16_t, PacketBufferTmSize>* packetsVCLength,
                                               etl::queue<uint16_t, PacketBufferTmSize>* framesSent,
                                               etl::queue<uint16_t, PacketBufferTmSize>* framesSentLength,
                                               uint8_t packetDestination2[TmTransferFrameSize], uint8_t vid) {
	uint8_t randomServicePicker2 = rand() % 2;
	ServiceChannelNotification ser;
	if (randomServicePicker2 == 0) {
		if (!framesSentLength->empty()) {
			*tmplength = framesSentLength->front();
			framesSentLength->pop();
			for (uint16_t t = 0; t < *tmplength; t++) {
				tmpData[t] = framesSent->front();
				framesSent->pop();
			}
			serviceChannel->allFramesReceptionTMRequest(tmpData, TmTransferFrameSize);
		}

	} else if (randomServicePicker2 == 1) {
		ser = serviceChannel->packetExtractionTM(vid, packetDestination2);
		if (ser == ServiceChannelNotification::NO_SERVICE_EVENT) {
			if (!packetsVCLength->empty()) {
				*flag = true;
				*tmplength = packetsVCLength->front();
				packetsVCLength->pop();
				for (uint16_t t = 0; t < *tmplength; t++) {
					tmpData[t] = packetsVC->front();
					packetsVC->pop();
				}
			}
			printf("Popping frame VC0 - length(%d): \n", *tmplength);
			printf("Buffer0: ");
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
	uint8_t packetDestination[TmTransferFrameSize] = {0};
	uint8_t packetDestination2[TmTransferFrameSize] = {0};
	uint8_t tmpData[TmTransferFrameSize] = {0};
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
	uint8_t tmplength = 0;
	ServiceChannelNotification ser;
	ServiceChannelNotification ser2;
	bool flag = false;
	uint16_t maximumServiceCallsTx = 100;
	uint16_t maximumServiceCallsRx = 50;
	uint16_t frameLength = maximumPacketLength;

	for (uint16_t i = 0; i < maximumServiceCallsTx; i++) {
		packetDestination[0] = 0;
		for (uint16_t j = 0; j < frameLength; j++) {
			frame[j] = i;
		}
		uint8_t virtualChannelPicker = rand() % 100;
		if (virtualChannelPicker <= 30) {
			ser2 = randomServiceCallTx(frameLength, frame, &serviceChannel, &packetsVC0, &packetsVC0Length, &framesSent,
			                           &framesSentLength, packetDestination, 0);
		} else if (virtualChannelPicker > 30 && virtualChannelPicker < 60) {
			ser2 = randomServiceCallTx(frameLength, frame, &serviceChannel, &packetsVC1, &packetsVC1Length, &framesSent,
			                           &framesSentLength, packetDestination, 1);
		} else {
			ser2 = randomServiceCallTx(frameLength, frame, &serviceChannel, &packetsVC2, &packetsVC2Length, &framesSent,
			                           &framesSentLength, packetDestination, 2);
		}

		if (ser2 == ServiceChannelNotification::NO_SERVICE_EVENT) {
			for (uint16_t l = 0; l < maximumServiceCallsRx; l++) {
				uint8_t virtualChannelPicker2 = rand() % 100;
				if (virtualChannelPicker2 <= 50) {
					randomServiceCallRx(&flag, tmpData, &tmplength, &serviceChannel, &packetsVC0, &packetsVC0Length,
					                    &framesSent, &framesSentLength, packetDestination2, 0);
				} else if (virtualChannelPicker2 > 50 && virtualChannelPicker2 < 60) {
					randomServiceCallRx(&flag, tmpData, &tmplength, &serviceChannel, &packetsVC1, &packetsVC1Length,
					                    &framesSent, &framesSentLength, packetDestination2, 1);
				} else {
					randomServiceCallRx(&flag, tmpData, &tmplength, &serviceChannel, &packetsVC2, &packetsVC2Length,
					                    &framesSent, &framesSentLength, packetDestination2, 2);
				}
				if (flag) {
					printf("\n -------------------------------------\n");
					printf("Packet temp: ");

					for (uint8_t k = 0; k < tmplength; k++) {
						printf("%d ", tmpData[k]);
						printf("%d\n", packetDestination2[k]);
						if (tmpData[k] != packetDestination2[k]) {
							numberOfErrors++;
							printf("FAILURE :");
						}
					}

					printf("\n");
					printf("Packet dest: ");

					for (uint8_t k = 0; k < tmplength; k++) {
						printf("%d ", packetDestination2[k]);
					}

					printf("\n");

					flag = false;
				}
				ser = ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
			}
		}
		ser2 = ServiceChannelNotification::VC_RX_WAIT_QUEUE_FULL;
	}
	CHECK(numberOfErrors == 0);
}
