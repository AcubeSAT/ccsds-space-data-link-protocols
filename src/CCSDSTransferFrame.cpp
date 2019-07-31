#include "CCSDSTransferFrame.hpp"

void CCSDSTransferFrame::createPrimaryHeader(uint8_t virtChannelID, bool secondaryHeaderPresent, bool ocfFlag) {
	uint16_t idOctets = 0; // The first two octets of the Primary header
	uint16_t dataFieldStatus = 0; // Hold the data field status

	idOctets |= static_cast<uint16_t>(ocfFlag); // Assign the Operational Control Flag
	idOctets |= (virtChannelID & 0x07U) << 1U; // Write the Virtual Channel ID in the Primary header
	idOctets |= (SPACECRAFT_IDENTIFIER & 0x03FFU) << 4U; // Append the spacecraft ID and the transfer frame version

	// Assign the ID header to the primary header
	primaryHeader.push_back((idOctets & 0xFF00U) >> 8U);
	primaryHeader.push_back(idOctets & 0x00FFU);

	primaryHeader.append(2, '\0');

	dataFieldStatus |= (static_cast<uint16_t>(secondaryHeaderPresent) & 0x0001U)
	                   << 15U; // Synch. flag and packet order flag are assumed zero
	dataFieldStatus |= 0x0003U << 11U; // Set the segment length ID to 11, as the standard recommends

	// Assign the data field status to the primary header
	primaryHeader.push_back((dataFieldStatus & 0xFF00U) >> 8U);
	primaryHeader.push_back(dataFieldStatus & 0x00FFU);
}
