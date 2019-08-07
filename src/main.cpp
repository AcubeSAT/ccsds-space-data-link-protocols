#include <iostream>
#include <CCSDSTransferFrameEncoder.hpp>

int main() {
	CCSDSTransferFrame transferFrame = CCSDSTransferFrame(4);
	CCSDSTransferFrameEncoder packet = CCSDSTransferFrameEncoder();

	String<MAX_PACKET_SIZE> encodedPacket;
	String<(MAX_PACKET_SIZE / (TRANSFER_FRAME_SIZE + SYNCH_BITS_SIZE)) * FRAME_DATA_FIELD_SIZE> data =
		String<(MAX_PACKET_SIZE / (TRANSFER_FRAME_SIZE + SYNCH_BITS_SIZE)) * FRAME_DATA_FIELD_SIZE>(
			"AFGDJ()982934HJVJHJLVUYVBJKAFGDJ()982934HJVJHJLVUYVBJKAFGDJ()9" \
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

	std::cout << "Data to append: " << data.c_str() << "\n\n";
	std::cout << "Master channel ID: " << std::hex << std::showbase
	          << unsigned(transferFrame.masterChannelID()) << std::endl;
	std::cout << "Virtual channel ID: " << std::hex << std::showbase
	          << unsigned(transferFrame.virtualChannelID()) << std::endl;

	std::cout << "Encoded frame data: ";
	for (auto const octet : encodedPacket) {
		std::cout << std::hex << std::showbase << unsigned(static_cast<uint8_t>(octet)) << " ";
	}
	std::cout << std::endl;

	return 0;
}
