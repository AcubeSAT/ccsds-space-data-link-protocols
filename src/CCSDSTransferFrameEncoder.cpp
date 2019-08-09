#include "CCSDSTransferFrameEncoder.hpp"

void CCSDSTransferFrameEncoder::encodeFrame(
    CCSDSTransferFrame& transferFrame,
    String<(MAX_PACKET_SIZE / (TRANSFER_FRAME_SIZE + SYNCH_BITS_SIZE)) * FRAME_DATA_FIELD_SIZE>& data,
    const uint32_t* packetSizes) {
	appendSynchBits();
	transferFrame.resetMasterChannelFrameCount();
	uint32_t count = 0;
	uint32_t index = 0;
	for (size_t i = 0; i < data.size(); i++) {
		if (packetSizes != nullptr) {
			if (count++ == packetSizes[index]) {
				transferFrame.setFirstHeaderPointer(
				    static_cast<uint16_t>((transferFrame.dataField.size() + 1) & 0x07FFU));
				index++;
				count = 0;
			}
		}

		if (transferFrame.dataField.size() == FRAME_DATA_FIELD_SIZE) {
			if (static_cast<uint8_t>(transferFrame.getPrimaryHeader().at(5)) == 0x00U) {
				transferFrame.setFirstHeaderPointer(0x07FFU); // No new packet is starting in frame
			}
			encodedFrame.append(transferFrame.transferFrame()); // Save the transfer frames in the packet
			transferFrame.increaseMasterChannelFrameCount(); // Increase the frame count
			appendSynchBits();

			transferFrame.dataField.clear(); // Get ready for the next data field iterations
		}
		transferFrame.dataField.push_back(data.at(i)); // Assign each octet to the encoded packet
	}

	// If the data field size is not full, make it
	if (transferFrame.dataField.size() < FRAME_DATA_FIELD_SIZE) {
		transferFrame.dataField.append(FRAME_DATA_FIELD_SIZE - transferFrame.dataField.size(), 0);
		transferFrame.setFirstHeaderPointer(0x07FEU); // Idle data contained
		encodedFrame.append(transferFrame.transferFrame()); // Save the transfer frames in the packet
		transferFrame.increaseMasterChannelFrameCount(); // Increase the frame count

		transferFrame.dataField.clear(); // Get ready for the next data field iterations
	}
}

void CCSDSTransferFrameEncoder::appendSynchBits() {
	encodedFrame.push_back(0x03U);
	encodedFrame.push_back(0x47U);
	encodedFrame.push_back(0x76U);
	encodedFrame.push_back(0xC7U);
	encodedFrame.push_back(0x27U);
	encodedFrame.push_back(0x28U);
	encodedFrame.push_back(0x95U);
	encodedFrame.push_back(0xB0U);
}
