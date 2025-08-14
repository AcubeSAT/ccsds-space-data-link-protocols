/**
 * @file AddressingAndParsingUtilities.hpp
 */

#pragma once
#include "CcsdsDefinitions.hpp"
#include "etl/tuple.h"

namespace CCSDSDataLinkLayer {
    inline Defs::VcidScidKey constructVcidScidKey(const Defs::Vcid vcid, const Defs::Scid scid) {
        return static_cast<Defs::VcidScidKey>(vcid) << 10 | scid;
    }

    inline Defs::MapidVcidScidKey constructMapidVcidScidKey(const Defs::Mapid mapid,
                                                      const Defs::Vcid vcid,
                                                      const Defs::Scid scid) {
        return (static_cast<Defs::MapidVcidScidKey>(mapid) << (6 + 10))
             | (static_cast<Defs::MapidVcidScidKey>(vcid) << 10)
             | scid;
    }

    inline etl::tuple<Defs::Vcid, Defs::Scid> extractVcidScid(Defs::VcidScidKey key) {
        Defs::Vcid vcid = static_cast<uint8_t>(key >> 10);   // top 6 bits
        Defs::Scid scid = static_cast<uint16_t>(key & 0x03FFU);
        return {vcid, scid};
    }

    inline etl::tuple<Defs::Mapid, Defs::Vcid, Defs::Scid> extractMapidVcidScidKey(Defs::MapidVcidScidKey key) {
        Defs::Mapid mapid = static_cast<uint8_t>(key >> (6 + 10));
        Defs::Vcid vcid  = static_cast<uint8_t>((key >> 10) & 0x3FU);
        Defs::Scid scid = static_cast<uint16_t>(key & 0x03FFU);
        return {mapid, vcid, scid};
    }

    /**
     * @brief Return the packet version number of a packet, if it is defined in the enumeration.
     */
    inline etl::optional<Defs::PacketVersionNumber> getPacketVersionNumber(uint8_t firstPacketOctet) {
        const uint8_t packetVersion = firstPacketOctet >> 5;
        if (packetVersion == static_cast<uint8_t>(Defs::PacketVersionNumber::SPACE_PACKET)) {
            return Defs::PacketVersionNumber::SPACE_PACKET;
        } else if (packetVersion == static_cast<uint8_t>(Defs::PacketVersionNumber::ENCAPSULATION_PACKET)) {
            return Defs::PacketVersionNumber::ENCAPSULATION_PACKET;
        } else {
            return etl::nullopt;
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
     * @brief Returns the total length of an encapsulation packer (as defined in CCSDS Encapsulation Packet Protocol).
     */
    inline uint32_t getEncapsulationPacketLength(const etl::span<uint8_t>& packetSource) {
        uint8_t lengthOfLengths = packetSource[0] & 0x03;
        if (lengthOfLengths == 0) {
            // no data field is present, total length is determined by packet header
        } else if (lengthOfLengths == 1) {
            // 1 octet length field
            return packetSource[Defs::EpTotalOffet];
        } else if (lengthOfLengths == 2) {
            // 2 octets length field
            return (static_cast<uint64_t>(packetSource[Defs::EpTotalOffet]) << 8) +
                packetSource[Defs::EpTotalOffet + 1];
        } else {
            // 4 octets length field
            return (static_cast<uint64_t>(packetSource[Defs::EpTotalOffet]) << 24) +
                (static_cast<uint64_t>(packetSource[Defs::EpTotalOffet + 1]) << 16) +
                (static_cast<uint64_t>(packetSource[Defs::EpTotalOffet + 2]) << 8) +
                packetSource[Defs::EpTotalOffet + 3];
        }
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