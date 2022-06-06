#include "catch2/catch.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSServiceChannel.hpp"
#include "CLCW.hpp"

TEST_CASE("Sending Tm Test"){

    PhysicalChannel physicalChannel = PhysicalChannel(1024, true, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels = {};

    MasterChannel masterChannel = MasterChannel(true);

    masterChannel.addVC(1, true, 1024, true, 32, 32, true, false, 64, true, FORWARD_ORDERED, 255,
                        10, 10, mapChannels);

    masterChannel.addVC(2, true, 1024, true, 32, 32, true, false, 0, false, OCTET, 255,
                        10, 10, mapChannels);
    masterChannel.addVC(3, true, 1024, false, 32, 32, false, false, 64, true, FORWARD_ORDERED, 255,
                        10, 10, mapChannels);
    ServiceChannel serviceChannel = ServiceChannel(masterChannel, physicalChannel);

    for(uint32_t i=0 ; i<50 ; i++){
        uint16_t packetLength = (rand() % 45 + 14);
        uint8_t packet[packetLength];
        for(uint8_t j =0 ; j < 6 ; j++){
            packet[j] = 0;
        }
        for(uint16_t j = 6; j < packetLength - 6 ; j++){
            packet[j] = rand() % 256;
        }
        uint8_t devider = rand() % 100;
        uint8_t packetData [TmTransferFrameSize];
        if(devider <= 50){
            serviceChannel.storeTM(packet, packetLength, 1);
            serviceChannel.mcGenerationTMRequest();
            serviceChannel.allFramesGenerationTMRequest(packetData, TmTransferFrameSize);
        }
        else if(devider > 50 && devider < 60){
            serviceChannel.storeTM(packet, packetLength, 2);
            serviceChannel.mcGenerationTMRequest();
            serviceChannel.allFramesGenerationTMRequest(packetData, TmTransferFrameSize);
        }
        else{
            serviceChannel.storeTM(packet, packetLength, 3);
            serviceChannel.mcGenerationTMRequest();
            serviceChannel.allFramesGenerationTMRequest(packetData, TmTransferFrameSize);
        }
    }

}