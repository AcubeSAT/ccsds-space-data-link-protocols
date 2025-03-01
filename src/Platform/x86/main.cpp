#include "FrameSender.hpp"
#include "CCSDSServiceChannel.hpp"
#include "FrameMaker.hpp"

namespace CCSDSDataLinkLayer {
    void manual(FrameMaker frameMaker) {
        // Choose virtual channel
        uint8_t vid = 0;

        // Feed packets to certain virtual channels (must be ecss packets)
        // TM(17,2)_are_you_alive_connection_test_report:
        // uint8_t packet1[] = {8, 1, 195, 39, 0, 10, 32, 17, 2, 1, 70, 0, 1, 37, 165, 61, 202}
        // TM(4,2)_parameter_statistics_report
        // uint8_t packet1[] = {8, 1, 195, 39, 0, 76, 32, 4, 2, 1, 70, 0, 1, 37, 165, 61, 202, 14, 224, 184, 148, 14, 224, 185, 92, 0, 2, 19, 152, 0, 3, 64, 160, 0, 0, 14, 224, 185, 92, 63, 128, 0, 0, 14, 224, 185, 92, 64, 64, 0, 0, 63, 209, 5, 236, 19, 153, 0, 6, 65, 80, 0, 0, 14, 224, 185, 92, 64, 64, 0, 0, 14, 224, 185, 92, 65, 0, 0, 0,0,0,0,0};

        uint8_t packet1[] = {8, 1, 195, 39, 0, 10, 32, 17, 2, 1, 70, 0, 1, 37, 165, 61, 202};
        uint8_t packet2[] = {8, 1, 195, 39, 0, 76, 32, 4, 2, 1, 70, 0, 1, 37, 165, 61, 202, 14, 224, 184, 148, 14, 224,
                             185, 92, 0, 2, 19, 152, 0, 3, 64, 160, 0, 0, 14, 224, 185, 92, 63, 128, 0, 0, 14, 224, 185,
                             92, 64, 64, 0, 0, 63, 209, 5, 236, 19, 153, 0, 6, 65, 80, 0, 0, 14, 224, 185, 92, 64, 64,
                             0, 0, 14, 224, 185, 92, 65, 0, 0, 0, 0, 0, 0, 0};

        frameMaker.packetFeeder(packet1, sizeof(packet1), vid);
        frameMaker.packetFeeder(packet2, sizeof(packet2), vid);

        // Feed CLCW
        uint32_t clcw = 0xFFFFFFFF;
        frameMaker.clcwFeeder(clcw, vid);

        // Transmit through certain virtual channels
        ServiceChannelNotification err;
        frameMaker.transmitChain(vid);

        printf("Frame Length: %d", frameMaker.getFrameLength());

        // Pop frames from queue and send them to yamcs
        // To simulate a client, open a terminal and write:
        // nc -v localhost 10014 | od -A d -t u1
        // If the full output is not shown, press enter
        FrameSender frameSender = FrameSender();
        uint8_t frame[TmTransferFrameSize] = {0};
        for (uint16_t i = 0; i < frameMaker.getNumberOfFramesSent(); ++i) {
            frameMaker.popWaitingFrame(frame);

            printf("Sent Frame: ");
            for (uint16_t j = 0; j < frameMaker.getFrameLength(); ++j) {
                printf(" %u ", frame[j]);
            }
            printf("\nPrimary Header Fields: \n"
                   "TFVN(%u)  SCID(%u) VID(%u)  OCF-FLAG(%u)  MC-FRAME-COUNT(%u)  VC-FRAME-COUNT(%u)\n"
                   "SEC-HEADER-FLAG(%u)  SYNCH-FLAG(%u)  PACKET-ORDER-FLAG(%u)  SEGMENT-ID(%u)  FIRST-HEADER-POINTER(%u)\n",
                   frame[0] & 0xC0,
                   ((static_cast<uint16_t>(frame[0] & 0x3F) << 4U) | (static_cast<uint16_t>(frame[1] & 0xF0) >> 4U)),
                   ((frame[1] & 0x0E)) >> 1U,
                   (frame[1]) & 0x01,
                   frame[2],
                   frame[3],
                   (frame[4] & 0x80) >> 7U,
                   (frame[4] & 0x40) >> 6U,
                   (frame[4] & 0x20) >> 5U,
                   (frame[4] >> 3) & 0x3,
                   ((static_cast<uint16_t>(((frame[4]) & 0x07)) << 8U) | (static_cast<uint16_t>((frame[5]))))
            );

            frameSender.sendFrameToYamcs(frame, frameMaker.getFrameLength());
            sleep(5);  // Do NOT remove or change this delay (or else consecutive frames might not be recognized)
        }
        //Print number of packet sent
        printf("Number of non Idle Packets Sent %d\n", frameMaker.getNumberOfPacketsSent());
    }

    void automatic(FrameMaker frameMaker) {
    }
} // namespace CCSDSDataLinkLayer


int main() {

    // Create Virtual, Master and Service Channels
    CCSDSDataLinkLayer::PhysicalChannel phy_channel_fop =
            CCSDSDataLinkLayer::PhysicalChannel(1024, 12, 1024, 220000, 20);

    etl::flat_map<uint8_t, CCSDSDataLinkLayer::MAPChannel, CCSDSDataLinkLayer::MaxMapChannels> map_channels = {
                {0, CCSDSDataLinkLayer::MAPChannel(0, true, true)},
                {1, CCSDSDataLinkLayer::MAPChannel(1, false, false)},
                {2, CCSDSDataLinkLayer::MAPChannel(2, true, false)},
        };

    CCSDSDataLinkLayer::MasterChannel master_channel = CCSDSDataLinkLayer::MasterChannel();
    uint8_t vcid1 = 0;
    bool segmentationV1 = true;
    bool blockingV1 = true;
    bool operationalControlFieldPresentV1 = false;
    bool errorControlFieldV1 = false;

    uint8_t vcid2 = 1;
    bool segmentationV2 = false;
    bool blockingV2 = false;
    bool operationalControlFieldPresentV2 = false;
    bool errorControlFieldV2 = false;

    uint8_t vcid3 = 2;
    bool segmentationV3 = false;
    bool blockingV3 = false;
    bool operationalControlFieldPresentV3 = false;
    bool errorControlFieldV3 = false;


    master_channel.addVC(vcid1, false, 128, blockingV1,
                         segmentationV1, false, errorControlFieldV1,
                         false, 0,
                         operationalControlFieldPresentV1,
                         CCSDSDataLinkLayer::SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED, 0,
                         0, 0, 1);
    master_channel.addVC(vcid2, false, 128, blockingV2,
                         segmentationV2, false, errorControlFieldV2,
                         false, 0,
                         operationalControlFieldPresentV2,
                         CCSDSDataLinkLayer::SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED, 0,
                         0, 0, 1);
    master_channel.addVC(vcid3, false, 128, blockingV3,
                         segmentationV3, false, errorControlFieldV3,
                         false, 0,
                         operationalControlFieldPresentV3,
                         CCSDSDataLinkLayer::SynchronizationFlag::OCTET_SYNCHRONIZED_FORWARD_ORDERED, 0,
                         0, 0, 1);


    CCSDSDataLinkLayer::ServiceChannel serv_channel =
            CCSDSDataLinkLayer::ServiceChannel(master_channel,
                                               phy_channel_fop,
                                               CCSDSDataLinkLayer::SecurityAssociation(),
                                               CCSDSDataLinkLayer::SecurityAssociation());

    // Create Frame Maker class instance, set frame data field length
    uint16_t transferFrameDataFieldLength = 17 + 7;   // ensure corresponding length in yamcs matches
    CCSDSDataLinkLayer::FrameMaker frameMaker = CCSDSDataLinkLayer::FrameMaker(&serv_channel, transferFrameDataFieldLength);

    // Call manual or automatic frame sending here
    manual(frameMaker);
    return 0;
}

