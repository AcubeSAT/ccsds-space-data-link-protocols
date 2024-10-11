#include "FrameSender.hpp"
#include "CCSDSServiceChannel.hpp"
#include "FrameMaker.hpp"

void manual(FrameMaker frameMaker){
    // Feed packets to certain virtual channels
    uint8_t packet1[] = {1, 2, 3, 4, 5, 6, 7};
    uint8_t packet2[] = {8, 9, 10, 11, 12};
    frameMaker.packetFeeder(packet1,sizeof(packet1) ,0);
    frameMaker.packetFeeder(packet2,sizeof(packet2) ,0);

    // Feed CLCW
    uint32_t clcw = 0xFFFFFFFF;
    frameMaker.clcwFeeder(clcw, 0);

    // Transmit through certain virtual channels
    ServiceChannelNotification err;
    frameMaker.transmitChain(0);

    printf("Frame Length: %d", frameMaker.getFrameLength());

    // Pop frames from queue and send them to yamcs
    // To simulate a client, open a terminal and write:
    // nc -l -v localhost 10014 | od -A d -t u1
    // If the full output is not shown, press enter
    FrameSender frameSender = FrameSender();
    uint8_t frame[TmTransferFrameSize] = {0};
    uint8_t sendAttempts = 1;
    for (uint16_t i = 0; i < frameMaker.getNumberOfFramesSent(); ++i) {
        frameMaker.popWaitingFrame(frame);

        printf("Sent Frame: ");
        for (uint16_t j = 0; j < frameMaker.getFrameLength(); ++j) {
            printf(" %d ", frame[j]);
        }
        printf("\n");

        for (uint16_t j = 0; j < sendAttempts; ++j) {
            frameSender.sendFrameToYamcs(frame, frameMaker.getFrameLength());
            sleep(2);
        }
    }
    //Print number of packet sent
    printf("Number of non Idle Packets Sent %d\n", frameMaker.getNumberOfPacketsSent());
}

void automatic(FrameMaker frameMaker){
}

int main(){

    // Create Virtual, Master and Service Channels
    PhysicalChannel phy_channel_fop = PhysicalChannel(1024, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, MAPChannel, MaxMapChannels> map_channels = {
            {0, MAPChannel(0, true, true)},
            {1, MAPChannel(1, false, false)},
            {2, MAPChannel(2, true, false)},
    };

    MasterChannel master_channel = MasterChannel();
    master_channel.addVC(0, true, 128, true, true, true,  2, 2, true, false, 8, true, SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED, 255, 10, 10, 3,
                         map_channels);

    ServiceChannel serv_channel = ServiceChannel(master_channel, phy_channel_fop);

    // Create Frame Maker class instance, set frame data field length
    uint16_t  transferFrameDataFieldLength = 10;
    FrameMaker frameMaker = FrameMaker(&serv_channel, transferFrameDataFieldLength);

    // Call manual or automatic frame sending here
    manual(frameMaker);
}

