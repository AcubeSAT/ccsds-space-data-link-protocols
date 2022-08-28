#include "catch2/catch.hpp"
#include "CCSDSChannel.hpp"
#include "CCSDSServiceChannel.hpp"
#include "CLCW.hpp"

TEST_CASE("Sending Tm"){

    PhysicalChannel physicalChannel = PhysicalChannel(1024, true, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels = {};

    MasterChannel masterChannel = MasterChannel();


    masterChannel.addVC(0, 128, true,3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 20, 20,
                         10);

	masterChannel.addVC(1, 128, false,3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 20, 20,
	                    10);

	masterChannel.addVC(2, 128, false,3, 2, false, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 3, 3,
	                    10);


    ServiceChannel serviceChannel = ServiceChannel(masterChannel, physicalChannel);
    uint8_t packetDestination[TmTransferFrameSize] = {0};

    //Initialize Services
    serviceChannel.initiateAdClcw(0);
    serviceChannel.initiateAdNoClcw(1);
    serviceChannel.initiateAdNoClcw(2);

    for(uint32_t i=0 ; i<50 ; i++){
        uint16_t packetLength = (rand() % 45 + 14);
        uint8_t packet[packetLength];
        for(uint8_t j =0 ; j < 6 ; j++){
            packet[j] = 0;
        }
        for(uint16_t j = 6; j < packetLength - 6 ; j++){
            packet[j] = rand() % 256;
        }
        uint8_t virtualChannelPicker = rand() % 100;
        if(virtualChannelPicker <= 50){
            uint8_t randomServicePicker = rand() % 4;
            if(randomServicePicker == 0){
                serviceChannel.storePacketTm(packet, packetLength, 0);
            }
            else if(randomServicePicker == 1){
                serviceChannel.vcGenerationService(60,0);
            }
            else if(randomServicePicker == 2){
                serviceChannel.mcGenerationTMRequest();
            }
            else if(randomServicePicker == 3){
                serviceChannel.allFramesGenerationTMRequest(packetDestination,TmTransferFrameSize);
            }
        }
        else if(virtualChannelPicker > 50 && virtualChannelPicker < 60){
            uint8_t randomServicePicker = rand() % 4;
            if(randomServicePicker == 0){
                serviceChannel.storePacketTm(packet, packetLength, 1);
            }
            else if(randomServicePicker == 1){
                serviceChannel.vcGenerationService(60,1);
            }
            else if(randomServicePicker == 2){
                serviceChannel.mcGenerationTMRequest();
            }
            else if(randomServicePicker == 3){
                serviceChannel.allFramesGenerationTMRequest(packetDestination,TmTransferFrameSize);
            }
        }
        else{
            uint8_t randomServicePicker = rand() % 4;
            if(randomServicePicker == 0){
                serviceChannel.storePacketTm(packet, packetLength, 2);
            }
            else if(randomServicePicker == 1){
                serviceChannel.vcGenerationService(60,2);
            }
            else if(randomServicePicker == 2){
                serviceChannel.mcGenerationTMRequest();
            }
            else if(randomServicePicker == 3){
                serviceChannel.allFramesGenerationTMRequest(packetDestination,TmTransferFrameSize);
            }
        }
    }
}

TEST_CASE("Receiving Tm"){
    PhysicalChannel physicalChannel = PhysicalChannel(1024, true, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> mapChannels = {};

    MasterChannel masterChannel = MasterChannel();


	masterChannel.addVC(0, 128, true,3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 20, 20,
	                    10);

	masterChannel.addVC(1, 128, false,3, 2, true, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 20, 20,
	                    10);

	masterChannel.addVC(2, 128, false,3, 2, false, false, 0, true, SynchronizationFlag::FORWARD_ORDERED,255 , 3, 3,
	                    10);

    ServiceChannel serviceChannel = ServiceChannel(masterChannel, physicalChannel);
    uint8_t packetDestination[TmTransferFrameSize] = {0};

    //Initialize Services
    serviceChannel.initiateAdClcw(0);
    serviceChannel.initiateAdNoClcw(1);
    serviceChannel.initiateAdNoClcw(2);
    uint16_t packetLength = (rand() % 45 + 14);
    uint8_t packet[packetLength];

    for(uint32_t i=0 ; i<50 ; i++) {
        for (uint16_t j = 0; j < TmTransferFrameSize; j++) {
            packet[j] = rand() % 256;
        }
    }
    uint8_t virtualChannelPicker = rand() % 100;
    if(virtualChannelPicker <= 50){
        uint8_t randomServicePicker = rand() % 2;
        if(randomServicePicker == 0){
            serviceChannel.allFramesReceptionTMRequest(packet,TmTransferFrameSize);
        }
        else if(randomServicePicker == 1){
            serviceChannel.packetExtractionTM(0, packetDestination);
        }
    }
    else if(virtualChannelPicker > 50 && virtualChannelPicker < 60){
        uint8_t randomServicePicker = rand() % 2;
        if(randomServicePicker == 0){
            serviceChannel.allFramesReceptionTMRequest(packet,TmTransferFrameSize);
        }
        else if(randomServicePicker == 1){
            serviceChannel.packetExtractionTM(1, packetDestination);
        }
    }
    else{
        uint8_t randomServicePicker = rand() % 2;
        if(randomServicePicker == 0){
            serviceChannel.allFramesReceptionTMRequest(packet,TmTransferFrameSize);
        }
        else if(randomServicePicker == 1){
            serviceChannel.packetExtractionTM(2, packetDestination);
        }
    }

}