#include "catch2/catch.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSServiceChannel.hpp"
#include "CLCW.hpp"
#include "stdlib.h"
TEST_CASE("Sending TM") {
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
	uint8_t trailerLength = 6;
	uint8_t maximumPacketLength = 20;
	uint8_t maximumFrameLength = maximumPacketLength + 12;
	uint8_t frame[TmTransferFrameSize];
	uint8_t headerLength = 6;
	uint8_t numberOfErrors = 0;
	uint8_t tmplength = 0;
	ServiceChannelNotification ser;
	ServiceChannelNotification ser2;
	bool flag = false;
	uint8_t maximumServiceCallsTx = 50;
	uint8_t maximumServiceCallsRx = 20;
	uint8_t frameLength = maximumPacketLength;

	for (uint8_t i = 0; i < maximumServiceCallsTx; i++) {
		packetDestination[0] = 0;
//		uint8_t frameLength = (rand() % maximumPacketLength);
		for (uint16_t j = 0; j < frameLength; j++) {
//			frame[j] = rand() % 256;
			frame[j] = i;
		}
		uint8_t virtualChannelPicker = rand() % 100;
		if (virtualChannelPicker <= 50) {
			uint8_t randomServicePicker = rand() % 4;
			if (randomServicePicker == 0) {

				printf("Push frame VC0 - length(%d): \n", frameLength);

				for (uint16_t i = 0; i < frameLength; i++){
					printf("%d ", frame[i]);
				}
				printf("\n");

				serviceChannel.storePacketTm(frame, frameLength, 0);
				packetsVC0Length.push(frameLength);
				for (int j = 0; j < frameLength; j++) {
					packetsVC0.push(frame[j]);
				}
                printf("Buffer0: ");
                for (size_t i = 0; i < packetsVC0.size(); i++)
                {
                    printf("%d ", packetsVC0.front());
                    packetsVC0.push(packetsVC0.front());
                    packetsVC0.pop();
                }
                printf("\n");
			} else if (randomServicePicker == 1) {
				serviceChannel.vcGenerationService(20, 0);
			} else if (randomServicePicker == 2) {
				serviceChannel.mcGenerationTMRequest();
			} else if (randomServicePicker == 3) {
				ser2 = serviceChannel.allFramesGenerationTMRequest(packetDestination, TmTransferFrameSize);

				if (ser2 == ServiceChannelNotification::NO_SERVICE_EVENT) {
					framesSentLength.push(TmTransferFrameSize);
					for (uint8_t j = 0; j < TmTransferFrameSize; j++) {
						framesSent.push(packetDestination[j]);
					}
					printf("Sent Frame(%d): \n", TmTransferFrameSize);
					for (uint16_t i = 0; i < TmTransferFrameSize; i++){
						printf("%d ", packetDestination[i]);
					}
				}
			}
		} else if (virtualChannelPicker > 50 && virtualChannelPicker < 60) {
			uint8_t randomServicePicker = rand() % 4;
			if (randomServicePicker == 0) {
                printf("Push frame VC1 - length(%d): \n", frameLength);
                for (uint16_t i = 0; i < frameLength; i++){
                    printf("%d ", frame[i]);
                }
                printf("\n");
				serviceChannel.storePacketTm(frame, frameLength, 1);
				packetsVC1Length.push(frameLength);
				for (int j = 0; j < frameLength; j++) {
					packetsVC1.push(frame[j]);
				}
                printf("Buffer1: ");
                for (size_t i = 0; i < packetsVC1.size(); i++)
                {
                    printf("%d ", packetsVC1.front());
                    packetsVC1.push(packetsVC1.front());
                    packetsVC1.pop();
                }
				printf("\n");
			} else if (randomServicePicker == 1) {
				serviceChannel.vcGenerationService(20, 1);
			} else if (randomServicePicker == 2) {
				serviceChannel.mcGenerationTMRequest();
			} else if (randomServicePicker == 3) {
				ser2 = serviceChannel.allFramesGenerationTMRequest(packetDestination, TmTransferFrameSize);
				if (ser2 == ServiceChannelNotification::NO_SERVICE_EVENT) {
					framesSentLength.push(TmTransferFrameSize);
					for (uint8_t j = 0; j < TmTransferFrameSize; j++) {
						framesSent.push(packetDestination[j]);
					}
					printf("Sent Frame(%d): \n", TmTransferFrameSize);
					for (uint16_t i = 0; i < TmTransferFrameSize; i++){
						printf("%d ", packetDestination[i]);
					}
				}
			}
		} else {
			uint8_t randomServicePicker = rand() % 4;
			if (randomServicePicker == 0) {
                printf("Push frame VC2 - length(%d): \n", frameLength);
                for (uint16_t i = 0; i < frameLength; i++){
                    printf("%d ", frame[i]);
                }
                printf("\n");
				serviceChannel.storePacketTm(frame, frameLength, 2);
				packetsVC2Length.push(frameLength);
				for (int j = 0; j < frameLength; j++) {
					packetsVC2.push(frame[j]);
				}
                printf("Buffer2: ");
                for (size_t i = 0; i < packetsVC2.size(); i++)
                {
                    printf("%d ", packetsVC2.front());
                    packetsVC2.push(packetsVC2.front());
                    packetsVC2.pop();
                }
                printf("\n");
            } else if (randomServicePicker == 1) {
				serviceChannel.vcGenerationService(20, 2);
			} else if (randomServicePicker == 2) {
				serviceChannel.mcGenerationTMRequest();
			} else if (randomServicePicker == 3) {
				ser2 = serviceChannel.allFramesGenerationTMRequest(packetDestination, TmTransferFrameSize);

				if (ser2 == ServiceChannelNotification::NO_SERVICE_EVENT) {
					framesSentLength.push(TmTransferFrameSize);
					for (int j = 0; j < TmTransferFrameSize; j++) {
						framesSent.push(packetDestination[j]);
					}
					printf("Sent Frame(%d): \n", TmTransferFrameSize);
					for (uint16_t i = 0; i < TmTransferFrameSize; i++){
						printf("%d ", packetDestination[i]);
					}
				}
			}
		}
		if (ser2 == ServiceChannelNotification::NO_SERVICE_EVENT) {
			for (int l = 0; l < maximumServiceCallsRx; l++) {
				uint8_t virtualChannelPicker2 = rand() % 100;
				if (virtualChannelPicker2 <= 50) {
					uint8_t randomServicePicker2 = rand() % 2;
					if (randomServicePicker2 == 0) {
                        if (!framesSentLength.empty()) {
							tmplength = framesSentLength.front();
							framesSentLength.pop();
							for (uint16_t t = 0; t < tmplength; t++) {
								tmpData[t] = framesSent.front();
								framesSent.pop();
							}
							serviceChannel.allFramesReceptionTMRequest(tmpData, TmTransferFrameSize);
						}
						printf("Frame received: ");
						for (size_t i = 0; i < TmTransferFrameSize; i++)
						{
							printf("%d ", tmpData[i]);
						}
						printf("\n");
					} else if (randomServicePicker2 == 1) {
						ser = serviceChannel.packetExtractionTM(0, packetDestination2);
						if (ser == ServiceChannelNotification::NO_SERVICE_EVENT) {
                            if (!framesSentLength.empty()) {
								flag = true;
								tmplength = packetsVC0Length.front();
								packetsVC0Length.pop();
								for (uint16_t t = 0; t < tmplength; t++) {
									tmpData[t] = packetsVC0.front();
									packetsVC0.pop();
								}
							}
                            printf("Popping frame VC0 - length(%d): \n", frameLength);
                            printf("Buffer0: ");
                            for (size_t i = 0; i < packetsVC0.size(); i++)
                            {
                                printf("%d ", packetsVC0.front());
                                packetsVC0.push(packetsVC0.front());
                                packetsVC0.pop();
                            }
                            printf("\n");
						}
					}
				} else if (virtualChannelPicker2 > 50 && virtualChannelPicker2 < 60) {
					uint8_t randomServicePicker2 = rand() % 2;
					if (randomServicePicker2 == 0) {
                        if (!framesSentLength.empty()) {
							tmplength = framesSentLength.front();
							framesSentLength.pop();
							for (uint16_t t = 0; t < tmplength; t++) {
								tmpData[t] = framesSent.front();
								framesSent.pop();
							}
							serviceChannel.allFramesReceptionTMRequest(tmpData, TmTransferFrameSize);

						}
						printf("Frame received: ");
						for (size_t i = 0; i < TmTransferFrameSize; i++)
						{
							printf("%d ", tmpData[i]);
						}
						printf("\n");
					} else if (randomServicePicker2 == 1) {
						ser = serviceChannel.packetExtractionTM(1, packetDestination2);
						if (ser == ServiceChannelNotification::NO_SERVICE_EVENT) {
							if (!packetsVC1Length.empty()) {
								flag = true;
								tmplength = packetsVC1Length.front();
								packetsVC1Length.pop();
								for (uint16_t t = 0; t < tmplength; t++) {
									tmpData[t] = packetsVC1.front();
									packetsVC1.pop();
								}
							}
                            printf("Popping frame VC1 - length(%d): \n", frameLength);
                            printf("Buffer1: ");
                            for (size_t i = 0; i < packetsVC1.size(); i++)
                            {
                                printf("%d ", packetsVC1.front());
                                packetsVC1.push(packetsVC1.front());
                                packetsVC1.pop();
                            }
                            printf("\n");
						}
					}
				} else {
					uint8_t randomServicePicker2 = rand() % 2;
					if (randomServicePicker2 == 0) {
                        if (!framesSentLength.empty()) {
							tmplength = framesSentLength.front();
							framesSentLength.pop();
							for (uint16_t t = 0; t < tmplength; t++) {
								tmpData[t] = framesSent.front();
								framesSent.pop();
							}
							serviceChannel.allFramesReceptionTMRequest(tmpData, TmTransferFrameSize);
						}
						printf("Frame received: ");
						for (size_t i = 0; i < TmTransferFrameSize; i++)
						{
							printf("%d ", tmpData[i]);
						}
						printf("\n");
					} else if (randomServicePicker2 == 1) {
						ser = serviceChannel.packetExtractionTM(2, packetDestination2);
						if (ser == ServiceChannelNotification::NO_SERVICE_EVENT) {
							flag = true;
							tmplength = packetsVC2Length.front();
							packetsVC2Length.pop();
							for (uint16_t t = 0; t < tmplength; t++) {
								tmpData[t] = packetsVC2.front();
								packetsVC2.pop();
							}
                            printf("Popping frame VC2 - length(%d): \n", frameLength);
                            printf("Buffer2: ");
                            for (size_t i = 0; i < packetsVC2.size(); i++)
                            {
                                printf("%d ", packetsVC2.front());
                                packetsVC2.push(packetsVC2.front());
                                packetsVC2.pop();
                            }
                            printf("\n");
						}
					}
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
