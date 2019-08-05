#include "CCSDSTransferFrame.hpp"

void CCSDSTransferFrame::createPrimaryHeader(bool secondaryHeaderPresent, bool ocfFlag) {
	uint16_t idOctets = 0; // The first two octets of the Primary header
	uint16_t dataFieldStatus = 0; // Hold the data field status

	idOctets |= static_cast<uint16_t>(ocfFlag); // Assign the Operational Control Flag
	idOctets |= (virtChannelID & 0x07U) << 1U; // Write the Virtual Channel ID in the Primary header
	idOctets |= (SPACECRAFT_IDENTIFIER & 0x03FFU) << 4U; // Append the spacecraft ID and the transfer frame version

	// Assign the ID header to the primary header
	primaryHeader.push_back((idOctets & 0xFF00U) >> 8U);
	primaryHeader.push_back(idOctets & 0x00FFU);

	primaryHeader.append(2, '\0'); // Set the master and virtual channel frame count to zero

	dataFieldStatus |= (static_cast<uint16_t>(secondaryHeaderPresent) & 0x0001U)
	                   << 15U; // Synch. flag and packet order flag are assumed zero
	dataFieldStatus |= 0x0003U << 11U; // Set the segment length ID to 11, as the standard recommends

	// Assign the data field status to the primary header
	primaryHeader.push_back((dataFieldStatus & 0xFF00U) >> 8U);
	primaryHeader.push_back(dataFieldStatus & 0x00FFU);
}

void CCSDSTransferFrame::increaseMasterChannelFrameCount() {
    auto currentCount = static_cast<uint8_t >(primaryHeader.at(2)); // Get the running count

    // Overflow check
    if (((currentCount % 256U) == 0U) && (currentCount <= 255U)) {
        currentCount++;
    } else {
        masterChannelOverflowFlag = true;
        currentCount = 0;
    }
    primaryHeader.insert(2, 1, static_cast<char >(currentCount)); // Append the updated value
}

void CCSDSTransferFrame::increaseVirtualChannelFrameCount() {
    auto currentCount = static_cast<uint8_t >(primaryHeader.at(3)); // Get the running count

    // Overflow check
    if (((currentCount % 256U) == 0U) && (currentCount <= 255U)) {
        currentCount++;
    } else {
        virtualChannelOverflowFlag = true;
        currentCount = 0;
    }
    primaryHeader.insert(3, 1, static_cast<char >(currentCount)); // Append the updated value
}

String<TRANSFER_FRAME_SIZE> CCSDSTransferFrame::transferFrame() {
    String<TRANSFER_FRAME_SIZE> completeFrame;

    for (auto octet : primaryHeader) {
        completeFrame.push_back(octet);
    }

#if SECONDARY_HEADER_SIZE > 0U
    for (auto octet: secondaryHeader) {
        completeFrame.push_back(octet);
    }
#endif

    for (auto const octet : dataField) {
        completeFrame.push_back(octet);
    }

    // Fill the rest of empty bits of the data field with zeros to maintain the length
    if (dataField.size() < FRAME_DATA_FIELD_MAX_SIZE) {
        completeFrame.append(FRAME_DATA_FIELD_MAX_SIZE - dataField.size(), '\0');
    }

    if (not operationalControlField.empty()) {
        for (auto const octet : operationalControlField) {
            completeFrame.push_back(octet);
        }
    }

    if (not errorControlField.empty()) {
        for (auto const octet : errorControlField) {
            completeFrame.push_back(octet);
        }
    }

    return completeFrame;
}

uint16_t CCSDSTransferFrame::getTransferFrameSize() {
    uint16_t tempSize = primaryHeader.size() + dataField.size() + operationalControlField.size();

#if SECONDARY_HEADER_SIZE > 0U
    tempSize += secondaryHeader.size()
#endif

    return tempSize;
}
