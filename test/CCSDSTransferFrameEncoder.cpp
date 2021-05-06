#include <catch2/catch.hpp>
#include <CCSDSTransferFrameEncoderTM.hpp>

TEST_CASE("CCSDS TMTransfer Frame Encoder") {
    CCSDSTransferFrameTM transferFrame = CCSDSTransferFrameTM(4);
    CCSDSTransferFrameEncoderTM packet = CCSDSTransferFrameEncoderTM();

    SECTION("Large data") {
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
        uint16_t pointerHeader = 0;
        uint16_t tempSum = packetSizes[0];
        uint16_t division = packetSizes[0] / tm_frame_data_field_size;

        packet.encodeFrame(transferFrame, data, packetSizes);
        encodedPacket.append(packet.getEncodedPacket());

        CHECK(encodedPacket.size() == 9 * tm_transfer_frame_size + 9 * tc_synch_bits_size);

        /*
         * Validate the primary headers in the packet
         * If the secondary header, or any other field is added do not forget to add it to the testing
         */
        for (size_t i = 0, j = 0, k = 0;
             i < encodedPacket.size(); i += tm_transfer_frame_size + tc_synch_bits_size, j++) {
            CHECK(static_cast<uint8_t>(encodedPacket.at(i)) == 0x03U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 1)) == 0x47U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 2)) == 0x76U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 3)) == 0xC7U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 4)) == 0x27U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 5)) == 0x28U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 6)) == 0x95U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 7)) == 0xB0U);

            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 8)) == 0x23U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 9)) == 0x78U);
            CHECK(static_cast<uint8_t>(encodedPacket.at(i + 10)) == j);

            // Check the data field contents
            if (i == 8 * (tm_transfer_frame_size + tc_synch_bits_size)) {
                // Final data length
                CHECK(encodedPacket.substr(i + 14, 77).compare(data.substr(5 * j, 77)) == 0);

                CHECK(static_cast<uint8_t>(encodedPacket.at(i + 12)) == 0x1FU);
                CHECK(static_cast<uint8_t>(encodedPacket.at(i + 13)) == 0xFEU);
            } else {
                CHECK(encodedPacket.substr(i + 14, tm_frame_data_field_size)
                              .compare(data.substr(5 * j, tm_frame_data_field_size)) == 0);

                if ((division-- > 0) && (packetSizes[k] > tm_transfer_frame_size)) {
                    CHECK(static_cast<uint8_t>(encodedPacket.at(i + 12)) == 0x1FU);
                    CHECK(static_cast<uint8_t>(encodedPacket.at(i + 13)) == 0xFFU);
                } else {
                    pointerHeader = tm_frame_data_field_size - ((j + 1U) * tm_frame_data_field_size - tempSum) + 1U;
                    division = packetSizes[++k] / tm_frame_data_field_size;
                    tempSum += packetSizes[k] + 1;

                    // Check the first header pointer for each packet
                    CHECK(
                            static_cast<uint8_t>(encodedPacket.at(i + 12)) ==
                            ((static_cast<uint8_t>(encodedPacket.at(i + 12)) & 0xF8U) |
                             ((pointerHeader & 0x0700U) >> 8U)));
                    CHECK(static_cast<uint8_t>(encodedPacket.at(i + 13)) ==
                          static_cast<uint8_t>((pointerHeader & 0x00FFU)));
                }
            }
        }
    }

    SECTION("Not so many data") {
        String<TM_MAX_PACKET_SIZE> encodedPacket;
        String<(TM_MAX_PACKET_SIZE / (tm_transfer_frame_size + tc_synch_bits_size)) * tm_frame_data_field_size> data =
                String<(TM_MAX_PACKET_SIZE / (tm_transfer_frame_size + tc_synch_bits_size)) * tm_frame_data_field_size>(
                        "AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934"
                        "HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJND");

        packet.encodeFrame(transferFrame, data);
        encodedPacket.append(packet.getEncodedPacket());

        CHECK(encodedPacket.size() ==
              tm_transfer_frame_size + tc_synch_bits_size); // Check that the size is the expected one

        CHECK(static_cast<uint8_t>(encodedPacket.at(0)) == 0x03U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(1)) == 0x47U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(2)) == 0x76U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(3)) == 0xC7U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(4)) == 0x27U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(5)) == 0x28U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(6)) == 0x95U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(7)) == 0xB0U);
        /*
         * Validate the primary headers in the packet
         * If the secondary header, or any other field is added do not forget to add it to the testing
         */
        CHECK(static_cast<uint8_t>(encodedPacket.at(8)) == 0x23U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(9)) == 0x78U);
        CHECK(static_cast<uint8_t>(encodedPacket.at(10)) == 0x00U);

        CHECK(static_cast<uint8_t>(encodedPacket.at(12)) == 0x1FU);
        CHECK(static_cast<uint8_t>(encodedPacket.at(13)) == 0xFEU);

        // Check the data field contents
        CHECK(encodedPacket.substr(14, tm_frame_data_field_size)
                      .compare("AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJ"
                               "VJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFG"
                               "DJ()982934HJVJHJLVUYVBJKG7894HJND"));
    }
}
