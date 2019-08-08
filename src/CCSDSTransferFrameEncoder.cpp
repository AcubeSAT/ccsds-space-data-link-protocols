#include "CCSDSTransferFrameEncoder.hpp"

void CCSDSTransferFrameEncoder::encodeFrame(CCSDSTransferFrame& transferFrame,
                                            String<(MAX_PACKET_SIZE / (TRANSFER_FRAME_SIZE + SYNCH_BITS_SIZE)) *
                                                   FRAME_DATA_FIELD_SIZE>& data) {
	appendSynchBits();
	transferFrame.resetMasterChannelFrameCount();
	for (auto const field : data) {
		if (transferFrame.dataField.size() == FRAME_DATA_FIELD_SIZE) {
			encodedFrame.append(transferFrame.transferFrame()); // Save the transfer frames in the packet
			transferFrame.increaseMasterChannelFrameCount(); // Increase the frame count
			appendSynchBits();

			transferFrame.dataField.clear(); // Get ready for the next data field iterations
		}
		transferFrame.dataField.push_back(field); // Assign each octet to the encoded packet
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
