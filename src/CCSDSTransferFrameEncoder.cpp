#include "CCSDSTransferFrameEncoder.hpp"

void CCSDSTransferFrameEncoder::encodeFrame(CCSDSTransferFrame& transferFrame,
                                            String<(MAX_PACKET_SIZE /
                                                    TRANSFER_FRAME_SIZE) * FRAME_DATA_FIELD_MAX_SIZE>& data) {
	for (auto const field : data) {
		if (transferFrame.dataField.size() == FRAME_DATA_FIELD_MAX_SIZE) {
			encodedFrame.append(transferFrame.transferFrame()); // Save the transfer frames in the packet
			transferFrame.dataField.clear(); // Get ready for the next data field iterations
			transferFrame.dataField.push_back(field);
		} else {
			transferFrame.dataField.push_back(field); // Assign each octet to the encoded packet
		}
	}

	// If the data field size is not full, make it
	if (transferFrame.dataField.size() < FRAME_DATA_FIELD_MAX_SIZE) {
		transferFrame.dataField.append(FRAME_DATA_FIELD_MAX_SIZE - transferFrame.dataField.size(), '\0');
		encodedFrame.append(transferFrame.transferFrame()); // Save the transfer frames in the packet
		transferFrame.dataField.clear(); // Get ready for the next data field iterations
	}
}
