#include "catch2/catch.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSServiceChannel.hpp"
#include "CLCW.hpp"

TEST_CASE("Sending TM"){

    PhysicalChannel physicalChannel = PhysicalChannel(1024, true, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels = {};

    MasterChannel masterChannel = MasterChannel();


    masterChannel.addVC(0, 128, true,3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 20, 20,
                         10);

	masterChannel.addVC(1, 128, false,3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 20, 20,
	                    10);

	masterChannel.addVC(2, 128, false,3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 3, 3,
	                    10);


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
	uint8_t maximumPacketLength = 45;
	uint8_t maximumFrameLength = maximumPacketLength+12;
	uint8_t frame[maximumFrameLength];
	uint8_t headerLength = 6;
	uint8_t numberOfErrors = 0;
	uint8_t headerAndTrailerLength = headerLength+trailerLength;
    for(uint8_t i=0 ; i<50 ; i++) {
		uint8_t frameLength = (rand() % maximumFrameLength);
		for (uint8_t j = 0; j < 6; j++) {
			frame[j] = 0;
		}
		for (uint16_t j = headerLength; j < frameLength - headerLength; j++) {
			frame[j] = rand() % 256;
		}
		uint8_t virtualChannelPicker = rand() % 100;
		if (virtualChannelPicker <= 50) {
			uint8_t randomServicePicker = rand() % 4;
			if (randomServicePicker == 0) {
				serviceChannel.storePacketTm(frame, frameLength, 0);
				packetsVC0Length.push(frameLength - headerAndTrailerLength);
				for (int j = 6; j < frameLength - 8; ++j) {
					packetsVC0.push(frame[j]);
				}
			} else if (randomServicePicker == 1) {
				serviceChannel.vcGenerationService(60, 0);
			} else if (randomServicePicker == 2) {
				serviceChannel.mcGenerationTMRequest();
			} else if (randomServicePicker == 3) {
				serviceChannel.allFramesGenerationTMRequest(packetDestination, TmTransferFrameSize);
				framesSentLength.push(frameLength);
				for (int j = 0; j < frameLength; ++j) {
					framesSent.push(packetDestination2[j]);
				}
			}
		} else if (virtualChannelPicker > 50 && virtualChannelPicker < 60) {
			uint8_t randomServicePicker = rand() % 4;
			if (randomServicePicker == 0) {
				serviceChannel.storePacketTm(frame, frameLength, 1);
				packetsVC1Length.push(frameLength - headerAndTrailerLength);
				for (int j = headerLength; j < frameLength - trailerLength; ++j) {
					packetsVC1.push(frame[j]);
				}
			} else if (randomServicePicker == 1) {
				serviceChannel.vcGenerationService(60, 1);
			} else if (randomServicePicker == 2) {
				serviceChannel.mcGenerationTMRequest();
			} else if (randomServicePicker == 3) {
				serviceChannel.allFramesGenerationTMRequest(packetDestination, TmTransferFrameSize);
				framesSentLength.push(frameLength);
				for (int j = 0; j < frameLength; ++j) {
					framesSent.push(packetDestination[j]);
				}
			}
		} else {
			uint8_t randomServicePicker = rand() % 4;
			if (randomServicePicker == 0) {
				serviceChannel.storePacketTm(frame, frameLength, 2);
				packetsVC2Length.push(frameLength - headerAndTrailerLength);
				for (int j = headerLength; j < frameLength - trailerLength; ++j) {
					packetsVC2.push(frame[j]);
				}
			} else if (randomServicePicker == 1) {
				serviceChannel.vcGenerationService(60, 2);
			} else if (randomServicePicker == 2) {
				serviceChannel.mcGenerationTMRequest();
			} else if (randomServicePicker == 3) {
				serviceChannel.allFramesGenerationTMRequest(packetDestination, TmTransferFrameSize);
				framesSentLength.push(frameLength);
				for (int j = 0; j < frameLength; ++j) {
					framesSent.push(packetDestination[j]);
				}
			}
		}
		if (packetDestination[0] != 0) {
			for (int l = 0; l < 10; l++) {
				uint8_t virtualChannelPicker2 = rand() % 100;
				if (virtualChannelPicker <= 50) {
					uint8_t randomServicePicker2 = rand() % 2;
					if (randomServicePicker2 == 0) {
						if(!framesSentLength.empty()) {
							uint8_t tmplength = framesSentLength.front();
							framesSentLength.pop();
							for (uint16_t t = 0; t < tmplength; t++) {
								tmpData[t] = framesSent.front();
								framesSent.pop();
							}
							serviceChannel.allFramesReceptionTMRequest(tmpData, TmTransferFrameSize);
						}
					} else if (randomServicePicker2 == 1) {
						serviceChannel.packetExtractionTM(0, packetDestination2);
						if (packetDestination2[0] != 0) {
							if (!packetsVC0Length.empty()) {
								uint8_t tmplength = packetsVC0Length.front();
								packetsVC0Length.pop();
								for (uint16_t t = 0; t < tmplength; t++) {
									tmpData[t] = packetsVC0.front();
									packetsVC0.pop();
								}
							}
						}
					}
				} else if (virtualChannelPicker > 50 && virtualChannelPicker < 60) {
					uint8_t randomServicePicker2 = rand() % 2;
					if (randomServicePicker2 == 0) {
						if (!framesSentLength.empty()) {
							uint8_t tmplength = framesSentLength.front();
							framesSentLength.pop();
							for (uint16_t t = 0; t < tmplength; t++) {
								tmpData[t] = framesSent.front();
								framesSent.pop();
							}
							serviceChannel.allFramesReceptionTMRequest(tmpData, TmTransferFrameSize);
						}

					} else if (randomServicePicker2 == 1) {
						serviceChannel.packetExtractionTM(1, packetDestination2);
						if (packetDestination2[0] != 0) {
							if (!packetsVC1Length.empty()) {
								uint8_t tmplength = packetsVC1Length.front();
								packetsVC1Length.pop();
								for (uint16_t t = 0; t < tmplength; t++) {
									tmpData[t] = packetsVC1.front();
									packetsVC1.pop();
								}
							}
						}
					}
				} else {
					uint8_t randomServicePicker2 = rand() % 2;
					if (randomServicePicker2 == 0) {
						if (!framesSentLength.empty()) {
							uint8_t tmplength = framesSentLength.front();
							framesSentLength.pop();
							for (uint16_t t = 0; t < tmplength; t++) {
								tmpData[t] = framesSent.front();
								framesSent.pop();
							}
							serviceChannel.allFramesReceptionTMRequest(tmpData, TmTransferFrameSize);
						}
					} else if (randomServicePicker2 == 1) {
						serviceChannel.packetExtractionTM(2, packetDestination2);
						if (packetDestination2[0] != 0) {
							if (!packetsVC2Length.empty()) {
								uint8_t tmplength = packetsVC2Length.front();
								packetsVC2Length.pop();
								for (uint16_t t = 0; t < tmplength; t++) {
									tmpData[t] = packetsVC2.front();
									packetsVC2.pop();
								}
							}
						}
					}
				}
				if (packetDestination2[0] != 0) {
					for (uint8_t k = 0; k < frameLength - headerAndTrailerLength; ++k) {
						if (tmpData[k] != packetDestination2[k]) {
							numberOfErrors++;
						}
					}
				}
			}
			packetDestination[0] = 0;
			packetDestination2[0] = 0;
		}
	}
	CHECK(numberOfErrors==0);
}
