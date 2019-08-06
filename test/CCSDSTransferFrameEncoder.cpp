#include <catch2/catch.hpp>
#include <CCSDSTransferFrameEncoder.hpp>
#include <iostream>

TEST_CASE("CCSDS Transfer Frame Encoder") {
	CCSDSTransferFrame transferFrame = CCSDSTransferFrame(4);
	CCSDSTransferFrameEncoder packet = CCSDSTransferFrameEncoder();

	SECTION("Large data") {
		String<MAX_PACKET_SIZE> encodedPacket;
		String<31232> data = String<31232>("AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()9" \
                                           "82934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982934" \
                                           "HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJK" \
                                           "AFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982934HJVJHJLVUYVBJKAFGDJ" \
                                           "()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJ" \
                                           "LVUYVBJKG7894HJNDAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYV" \
                                           "BJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAF" \
                                           "GDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJV" \
                                           "JHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982934HJVJHJLV" \
                                           "UYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()9" \
                                           "82934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934" \
                                           "HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJK" \
                                           "G7894HJNDAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ" \
                                           "()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJNDAFGDJ()982" \
                                           "934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYV" \
                                           "BJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJND");

		packet.encodeFrame(transferFrame, data);
		encodedPacket.append(packet.getEncodedPacket());

		CHECK(encodedPacket.size() == 9 * TRANSFER_FRAME_SIZE); // Check that the size corresponds to the expected one

		/*
		 * Validate the primary headers in the packet
		 * If the secondary header, or any other field is added do not forget to add it to the testing
		 */
		for (size_t i = 0, j = 0; i < encodedPacket.size(); i += TRANSFER_FRAME_SIZE, j++) {
			CHECK(encodedPacket.at(i) == 0x23U);
			CHECK(encodedPacket.at(i + 1) == 0x78U);
			CHECK(encodedPacket.at(i + 2) == j);

			CHECK(encodedPacket.at(i + 4) == 0x18U);
			CHECK(encodedPacket.at(i + 5) == 0x00U);


			// Check the data field contents
			if (i == 8 * TRANSFER_FRAME_SIZE) {
				// Final data length
				CHECK(encodedPacket.substr(i + 6, 77).compare(data.substr(5 * j, 77)) == 0);
			} else {
				CHECK(encodedPacket.substr(i + 6, FRAME_DATA_FIELD_MAX_SIZE).compare(
					data.substr(5 * j, FRAME_DATA_FIELD_MAX_SIZE)) == 0);
			}
		}
	}

	SECTION("Not so many data") {
		String<MAX_PACKET_SIZE> encodedPacket;
		String<31232> data = String<31232>("AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934" \
                                           "HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKG7894HJND");

		packet.encodeFrame(transferFrame, data);
		encodedPacket.append(packet.getEncodedPacket());

		CHECK(encodedPacket.size() == TRANSFER_FRAME_SIZE); // Check that the size corresponds to the expected one

		/*
		 * Validate the primary headers in the packet
		 * If the secondary header, or any other field is added do not forget to add it to the testing
		 */
		CHECK(encodedPacket.at(0) == 0x23U);
		CHECK(encodedPacket.at(1) == 0x78U);
		CHECK(encodedPacket.at(2) == 0U);

		CHECK(encodedPacket.at(4) == 0x18U);
		CHECK(encodedPacket.at(5) == 0x00U);

		// Check the data field contents
		CHECK(encodedPacket.substr(6, FRAME_DATA_FIELD_MAX_SIZE).compare("AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJ" \
                                                                         "VJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFG" \
                                                                         "DJ()982934HJVJHJLVUYVBJKG7894HJND"));
	}
}
