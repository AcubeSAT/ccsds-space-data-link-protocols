#include <iostream>
#include <CCSDSTransferFrameEncoderTM.hpp>

int main() {
    CCSDSTransferFrameTM transferFrame = CCSDSTransferFrameTM(4);
    CCSDSTransferFrameEncoderTM packet = CCSDSTransferFrameEncoderTM();


    String<TM_MAX_PACKET_SIZE> encodedPacket;
    String<(TM_MAX_PACKET_SIZE / (tm_transfer_frame_size + tc_synch_bits_size)) * tm_frame_data_field_size> data =
            String<(TM_MAX_PACKET_SIZE / (tm_transfer_frame_size + tc_synch_bits_size)) * tm_frame_data_field_size>(
                    "AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()9"
                    "82934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982934"
                    "HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJK"
                    "AFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982934HJVJHJLVUYVBJKAFGDJ"
                    "()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJ"
                    "LVUYVBJKG7894HJNDAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYV"
                    "BJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAF"
                    "GDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJV"
                    "JHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982934HJVJHJLV"
                    "UYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()9"
                    "82934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934"
                    "HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJK"
                    "G7894HJNDAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ"
                    "()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982"
                    "934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYV"
                    "BJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJND");
    uint32_t packetSizes[7] = {351, 117, 117, 117, 127, 105, 119};

    packet.encodeFrame(transferFrame, data, packetSizes);
    encodedPacket.append(packet.getEncodedPacket());

    std::cout << "Data to append: " << data.c_str() << "\n\n";
    std::cout << "Master channel ID: " << std::hex << std::showbase << unsigned(transferFrame.masterChannelID())
              << std::endl;
    std::cout << "Virtual channel ID: " << std::hex << std::showbase << unsigned(transferFrame.virtualChannelID())
              << std::endl;

    std::cout << "Encoded frame data: ";
    for (auto const octet : encodedPacket) {
        std::cout << std::hex << std::showbase << unsigned(static_cast<uint8_t>(octet)) << " ";
    }
    std::cout << std::endl;

    return 0;
}
