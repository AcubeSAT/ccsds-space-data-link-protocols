#include <catch2/catch.hpp>
#include <CCSDSTransferFrameEncoderTM.hpp>

TEST_CASE("CCSDS TMTransfer Frame Encoder") {
    CCSDSTransferFrameTM transferFrame = CCSDSTransferFrameTM(4);
    CCSDSTransferFrameEncoderTM packet = CCSDSTransferFrameEncoderTM();

    SECTION("Large data") {
        String<TM_MAX_PACKET_SIZE> encodedPacket;
        String<(TM_MAX_PACKET_SIZE / (TM_TRANSFER_FRAME_SIZE + TC_SYNCH_BITS_SIZE)) * TM_FRAME_DATA_FIELD_SIZE> data =
                String<(TM_MAX_PACKET_SIZE / (TM_TRANSFER_FRAME_SIZE + TC_SYNCH_BITS_SIZE)) * TM_FRAME_DATA_FIELD_SIZE>(
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
        uint16_t division = packetSizes[0] / TM_FRAME_DATA_FIELD_SIZE;

        packet.encodeFrame(transferFrame, data, packetSizes);
        encodedPacket.append(packet.getEncodedPacket());

        CHECK(encodedPacket.size() == 9 * TM_TRANSFER_FRAME_SIZE + 9 * TC_SYNCH_BITS_SIZE);

        /*
         * Validate the primary headers in the packet
         * If the secondary header, or any other field is added do not forget to add it to the testing
         */
        for (size_t i = 0, j = 0, k = 0;
             i < encodedPacket.size(); i += TM_TRANSFER_FRAME_SIZE + TC_SYNCH_BITS_SIZE, j++) {
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
            if (i == 8 * (TM_TRANSFER_FRAME_SIZE + TC_SYNCH_BITS_SIZE)) {
                // Final data length
                CHECK(encodedPacket.substr(i + 14, 77).compare(data.substr(5 * j, 77)) == 0);

                CHECK(static_cast<uint8_t>(encodedPacket.at(i + 12)) == 0x1FU);
                CHECK(static_cast<uint8_t>(encodedPacket.at(i + 13)) == 0xFEU);
            } else {
                CHECK(encodedPacket.substr(i + 14, TM_FRAME_DATA_FIELD_SIZE)
                              .compare(data.substr(5 * j, TM_FRAME_DATA_FIELD_SIZE)) == 0);

                if ((division-- > 0) && (packetSizes[k] > TM_TRANSFER_FRAME_SIZE)) {
                    CHECK(static_cast<uint8_t>(encodedPacket.at(i + 12)) == 0x1FU);
                    CHECK(static_cast<uint8_t>(encodedPacket.at(i + 13)) == 0xFFU);
                } else {
                    pointerHeader = TM_FRAME_DATA_FIELD_SIZE - ((j + 1U) * TM_FRAME_DATA_FIELD_SIZE - tempSum) + 1U;
                    division = packetSizes[++k] / TM_FRAME_DATA_FIELD_SIZE;
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
        String<(TM_MAX_PACKET_SIZE / (TM_TRANSFER_FRAME_SIZE + TC_SYNCH_BITS_SIZE)) * TM_FRAME_DATA_FIELD_SIZE> data =
                String<(TM_MAX_PACKET_SIZE / (TM_TRANSFER_FRAME_SIZE + TC_SYNCH_BITS_SIZE)) * TM_FRAME_DATA_FIELD_SIZE>(
                        "AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934"
                        "HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJND");

        packet.encodeFrame(transferFrame, data);
        encodedPacket.append(packet.getEncodedPacket());

        CHECK(encodedPacket.size() ==
              TM_TRANSFER_FRAME_SIZE + TC_SYNCH_BITS_SIZE); // Check that the size is the expected one

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
        CHECK(encodedPacket.substr(14, TM_FRAME_DATA_FIELD_SIZE)
                      .compare("AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJ"
                               "VJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFG"
                               "DJ()982934HJVJHJLVUYVBJKG7894HJND"));
    }
}
