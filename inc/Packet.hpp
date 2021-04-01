#ifndef CCSDS_PACKET_H
#define CCSDS_PACKET_H

#include <CCSDS_Definitions.hpp>
#include <algorithm>
#include <cstring>

enum ServiceType {
    TYPE_A = 0,
    TYPE_B = 1
};

enum FDURequestType {
    REQUEST_PENDING = 0,
    REQUEST_CONFIRMED = 1,
    REQUEST_DENIED = 2,
};

struct Packet {
    // @todo those fields shouldn't be public
    uint8_t *packet;
    uint16_t packetLength;
    uint8_t segHdr;
    uint8_t gvcid;
    uint8_t mapid;
    uint16_t sduid;
    ServiceType serviceType;
    uint8_t transferFrameSeqNumber;
    bool acknowledged;

    void setConfSignal(FDURequestType reqSignal) {
        confSignal = reqSignal;
        // TODO Maybe signal the higher procedures here instead of having them manually take care of them
    }

    /**
     * @brief Determines whether the packet is marked for retransmission while in the sent queue
     */
    const bool to_be_retransmitted() const {
        return toBeRetransmitted;
    }

    void mark_for_retransmission(bool f) { toBeRetransmitted = f; }

    Packet(uint8_t *packet, uint16_t packet_length, uint8_t seg_hdr, uint8_t gvcid, uint8_t mapid, uint16_t sduid,
           ServiceType service_type) :
            packet(packet), packetLength(packet_length), segHdr(seg_hdr), gvcid(gvcid), mapid(mapid), sduid(sduid),
            serviceType(service_type), transferFrameSeqNumber(0), acknowledged(0), toBeRetransmitted(0) {}

private:
    bool toBeRetransmitted;
    // This is used by COP to signal the higher procedures
    FDURequestType confSignal;
};

class CLCW {
public:
    void setConfSignal(FDURequestType reqSignal) {
        confSignal = reqSignal;
        // TODO Maybe signal the higher procedures here instead of having them manually take care of them
    }

    /**-+
     * @brief Field status is mission-specific and not used in the CCSDS data link protocol
     */
    const uint8_t field_status() const {
        return (clcw_data[0] & 0x1C) >> 2U;
    }

    /**
     * @brief COP version in effect
     */
    const uint8_t cop_in_effect() const {
        return clcw_data[0] & 0x03;
    }

    /**
     * @brief Virtual Channel identifier of target virtual channel
     */
    const uint8_t vcid() const {
        return (clcw_data[1] & 0xFC) >> 2U;
    }

    /**
     * @brief Indicates whether RF transmission in the physical channel is ready
     */
    const bool no_rf_avail() const {
        return (clcw_data[2] & 0x80) >> 7U;
    }

    /**
     * @brief Indicates whether "bit lock" is achieved
     */
    const bool no_bit_lock() const {
        return (clcw_data[2] & 0x40) >> 6U;
    }

    /**
     * @brief Indicates whether FARM is in the Lockout state
     */
    const bool lockout() const {
        return (clcw_data[2] & 0x20) >> 5U;
    }

    /**
     * @brief Indicates whether the receiver doesn't accept any packets
     */
    const bool wait() const {
        return (clcw_data[2] & 0x10) >> 4U;
    }

    /**
     * @brief Indicates whether there are Type-A Frames that will need to be retransmitted
     */
    const bool retransmission() const {
        return (clcw_data[2] & 0x08) >> 3U;
    }

    /**
     * @brief The two least significant bits of the virtual channel
     */
    const uint8_t farm_b_counter() const {
        return (clcw_data[2] & 0x06) >> 1U;
    }

    /**
     * @brief Next expected transfer frame number
     */
    const uint8_t report_value() const {
        return clcw_data[3];
    }

    // @todo Is this a bit risky, ugly and hacky? Yes, like everything in life. However, I still prefer this over
    //  passing etl strings around but that's probably just me.

    /**
     * @brief Control Link Words used for FARM reports
     */
    CLCW(uint8_t *data) {
        clcw_data[0] = data[0];
        clcw_data[1] = data[1];
        clcw_data[2] = data[2];
        clcw_data[3] = data[3];
    }

    /**
     * @brief Control Link Words used for FARM reports
     */
    CLCW(const uint8_t field_status, const uint8_t cop_in_effect, const uint8_t virtual_channel, const bool no_rf_avail,
         const bool no_bit_lock, const bool lockout, const bool wait, const bool retransmit,
         const uint8_t farm_b_counter,
         const uint8_t report_value) {
        clcw_data[0] = (field_status << 2U) | cop_in_effect;
        clcw_data[1] = (virtual_channel << 2U);
        clcw_data[2] = (no_rf_avail << 7U) | (no_bit_lock << 6U) | (lockout << 5U) | (wait << 4U) |
                       (retransmit << 3U) | (farm_b_counter << 1U);
        clcw_data[3] = report_value;
    }

private:
    uint8_t clcw_data[4];
    // This is used by COP to signal the higher procedures
    FDURequestType confSignal;
};

#endif //CCSDS_PACKET_H
