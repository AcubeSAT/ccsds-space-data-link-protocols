#include <catch2/catch.hpp>
#include <CCSDSTransferFrameEncoder.hpp>

TEST_CASE("CCSDS Transfer Frame Encoder") {
    CCSDSTransferFrame transferFrame = CCSDSTransferFrame(4);
    CCSDSTransferFrameEncoder packet = CCSDSTransferFrameEncoder();

    String<256> data = String<256>("AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLV" \
                                   "UYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJND");
    data.append(138, '\0');
    packet.encodeFrame(transferFrame, data);

    String<MAX_PACKET_SIZE> encodedPacket;
    encodedPacket.append(packet.getEncodedPacket());
}
