/**
 * @file AddressingAndParsingUtilities.hpp
 */

#pragma once
#include "CcsdsDefinitions.hpp"

namespace CCSDSDataLinkLayer {
    inline Defs::VcidScidKey constructVcidScidKey(const uint8_t vcid, const uint16_t scid) {
        return static_cast<Defs::VcidScidKey>(vcid) << 10 | scid;
    }

    inline Defs::MapidVcidScidKey constructMapidVcidScidKey(const uint8_t mapid,
                                                      const uint8_t vcid,
                                                      const uint16_t scid) {
        return (static_cast<Defs::MapidVcidScidKey>(mapid) << (6 + 10))
             | (static_cast<Defs::MapidVcidScidKey>(vcid) << 10)
             | scid;
    }

    inline etl::tuple<uint8_t, uint16_t> extractVcidScid(Defs::VcidScidKey key) {
        uint8_t vcid = static_cast<uint8_t>(key >> 10);   // top 6 bits
        uint16_t scid = static_cast<uint16_t>(key & 0x03FFU);
        return {vcid, scid};
    }

    inline etl::tuple<uint8_t, uint8_t, uint16_t> extractMapidVcidScidKey(Defs::MapidVcidScidKey key) {
        uint8_t mapid = static_cast<uint8_t>(key >> (6 + 10));
        uint8_t vcid  = static_cast<uint8_t>((key >> 10) & 0x3FU);
        uint16_t scid = static_cast<uint16_t>(key & 0x03FFU);
        return {mapid, vcid, scid};
    }

    /**
     * @brief Return the packet version number of a packet, if it is defined in the enumeration.
     */
    inline etl::optional<Defs::PacketVersionNumber> getPacketVersionNumber(uint8_t firstPacketOctet) {
        if (const uint8_t packetVersion = firstPacketOctet >> 5; packetVersion == static_cast<uint8_t>(Defs::PacketVersionNumber::SPACE_PACKET)) {
            return Defs::PacketVersionNumber::SPACE_PACKET;
        } else {
            return Defs::PacketVersionNumber::ENCAPSULATION_PACKET;
        }
    }

    /**
     * @brief Returns the total length of a space packet (as defined in CCSDS Space Packet Protocol).
     */
    inline uint16_t getSpacePacketLength(const etl::span<uint8_t>& packetSource) {
        // plus one is added because the field actually contains the data field length, reduced by one
        return (static_cast<uint16_t>(packetSource[Defs::SpacePacketDataLengthFieldPosition - 1]) << 8) |
               (static_cast<uint16_t>(packetSource[Defs::SpacePacketDataLengthFieldPosition])) + Defs:: SpacePacketPrimaryHeaderLength + 1;
    }

    /**
     * TODO
     */
    inline uint32_t getEncapsulationPacketLength(const uint8_t *packetSource) {

    }

    /**
     * Due to the COP-1 arithmetic being mod 256, it is possible in the inequality:
     * lowerBound <= value <= upperBound
     * for upperBound to be numerically smaller than lower bound (wraparound).
     *
     * @returns True, if the given value is within the window or it's edges. Otherwise, false is returned.
     */
    inline bool withinWindow(const uint8_t value, const uint8_t lowerBound, const uint8_t upperBound) {
        if (upperBound < lowerBound) { // wraparound
            // The window region consists of 2 subregions: [lowerBound, 255] and [0, upperBound]
            return ((value >= lowerBound) && (value <= 255)) || (value <= upperBound);
        } else {  // normal comparison
            return (value >= lowerBound) && (value <= upperBound);
        }
    }

} // CCSDSDataLinkLayer