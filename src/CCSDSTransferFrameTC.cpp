#include "CCSDSTransferFrameTC.hpp"

void CCSDSTransferFrameTC::createPrimaryHeader() {
    uint16_t idOctet = 0; // The first two octets of the header
    uint16_t dataFieldStatus = 0; // Hold the data field status

    idOctet |= static_cast<uint16_t>(spacecraft_identifier) & 0x7FF; // Append the Spacecraft ID
    idOctet |= static_cast<uint16_t>(ctrlCmdFlag & 0x01) << 12U; // Set the Control and Command Flag
    idOctet |= static_cast<uint16_t>(bypassFlag & 0x01) << 13U; // Set the Bypass Flag
    // Transfer Frame Version Number is set to 00

    primaryHeader.push_back((idOctet & 0xFF00U) >> 8U);
    primaryHeader.push_back(idOctet & 0x00FFU);

    uint16_t id2Octet = 0; // Second pair of octets of the header
    id2Octet |= static_cast<uint16_t>(frameLength & 0x3FF); // Append Frame Length
    id2Octet |= static_cast<uint16_t>(virtChannelID & 0x3F) << 10U; // Append virtual channel ID

    primaryHeader.push_back((id2Octet & 0xFF00U) >> 8U);
    primaryHeader.push_back(id2Octet & 0x00FFU);

    primaryHeader.push_back(frameSeqCount); // Append Frame Sequence Counter
}

String<tc_max_header_size> CCSDSTransferFrameTC::transferFrame() {
    String<tc_max_header_size> completeFrame;

    for (uint8_t const octet : primaryHeader) {
        completeFrame.push_back(octet);
    }

    uint8_t i = 0;
    for (uint8_t const octet:dataField) {
        completeFrame.push_back(octet);
        if (i++ == frameLength - 5 - 2 * tc_error_control_field_exists) {
            break;
        }
    }

    return completeFrame;
}

void CCSDSTransferFrameTC::incrementFrameSequenceCount() {
    if (frameSeqCount == 0xFF) {
        frameSeqCount = 0;
    } else {
        frameSeqCount++;
    }
    primaryHeader.replace(4, 1, 1, frameSeqCount);
}

void CCSDSTransferFrameTC::setFrameSequenceCount(uint8_t count) {
    frameSeqCount = count;

    primaryHeader.replace(4, 1, 1, frameSeqCount);
}