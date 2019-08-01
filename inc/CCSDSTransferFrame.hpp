#ifndef CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP
#define CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP

#include "CCSDS_Definitions.hpp"
#include "etl/String.hpp"

/**
 * @brief Define a CCSDS packet with the required attributes
 *
 * @todo Define if the Operational Control Field and the Secondary header are required for our project
 */
class CCSDSTransferFrame {
private:
    /**
     * @brief Internal function to create the packet's primary header upon the packet creation.
     * @details This particular function is called inside the constructor to generate the primary header of the packet.
     * The primary header is saved as the `String<PRIMARY_HEADER_SIZE>` attribute in this class and can be accessed publicly.
     *
     * @note As per CCSDS 132.0-B-2 recommendation, the transfer frame version number is `00`
     * @attention The synchronization flag and the packet order flag of the data field status, in the primary header,
     * are assumed to be `0` in this implementation, thus requiring that the segment length ID is `11`.
     */
    void createPrimaryHeader(uint8_t virtChannelID, bool secondaryHeaderPresent, bool ocfFlag);

    /**
     * @brief Generates the transfer frame secondary header
     *
     * @attention At the moment the secondary header part is not implemented, since it is not required.
     */
#if SECONDARY_HEADER_SIZE > 0U
    void createSecondaryHeader(String<SECONDARY_HEADER_SIZE - 1>& dataField);
#endif

    /**
     *
     */
    uint8_t mChannelFrameCount;

    /**
     *
     */
    uint8_t vChannelFrameCount;

    /**
     *
     */
    uint8_t virtChannelID;

protected:
    // Field information containers
    /**
     * @brief Store the transfer frame primary header generated from the `createPrimaryHeader` function
     */
    String<PRIMARY_HEADER_SIZE> primaryHeader;

    /**
     * @brief Store the transfer frame secondary header generated from the `createSecondaryHeader` function
     *
     * @attention At the moment the secondary header part is not implemented, since it is not required.
     */
#if SECONDARY_HEADER_SIZE > 0U
    String<SECONDARY_HEADER_SIZE> secondaryHeader;
#endif

    /**
     * @brief Hold the data field octets
     * @details The length of this field is variable and it is determined by the mission requirements for the
     * total packet length.
     */
    String<FRAME_DATA_FIELD_MAX_SIZE> dataField;

    /**
     * @brief Store the operational control field octets
     *
     * @attention For the moment the operational control field is not implemented and the usage is TBD
     */
    String<4> operationalControlField;

    /**
     * @brief Store the result of the CRC generation for the packet as the standard recommends
     * @attention This will not be implemented for our mission, since other error correcting and detection techniques
     * will be implemented
     */
    String<2> errorControlField;

    // Counters and single variables
    /**
     * @brief Hold the data field total size
     */
    uint8_t dataFieldSize;

public:
    /**
     *
     *
     * @param datFieldSize Provide the total length in octets of the data field
     * @param virtChannelID Provide the virtual channel ID, if used, otherwise it is zero by default
     */
    explicit CCSDSTransferFrame(uint8_t datFieldSize, uint8_t vChannelID = 0)
            : dataFieldSize(datFieldSize), primaryHeader(),
#if SECONDARY_HEADER_SIZE > 0U
            secondaryHeader(),
#endif
              dataField(), operationalControlField(), errorControlField(), mChannelFrameCount(0), vChannelFrameCount(0),
              virtChannelID(vChannelID) {
        createPrimaryHeader(vChannelID, SECONDARY_HEADER_SIZE > 0U, not operationalControlField.empty());
    }

    /**
     * @brief Increase the master channel frame count
     * @details The master channel frame count field provides the running count of the frames transmitted through the
     * same master channel. Since the field is 8-bit, care should be taken to avoid overflow and also as per the
     * standard's recommendation, re-setting of the count before reaching the value of 255 should be avoided.
     */
    void increaseMasterChannelFrameCount() {};

    /**
     * @brief Decrease the master channel frame count
     */
    void decreaseMasterChannelFrameCount() {};

    /**
     * @brief Increase the virtual channel frame count
     * @details The virtual channel frame count field provides the individual accountability for each virtual channel,
     * to enable systematic packet extraction from the transfer frame data field. Same as the master count, re-setting
     * before 255 should be avoided and the count is again 8-bit.
     */
    void increaseVirtualChannelFrameCount() {};

    /**
     * @brief Decrease the virtual channel frame count
     */
    void decreaseVirtualChannelFrameCount() {};

    /**
     * @brief Set the 11-bit first header pointer in the data field status of the primary header
     * @details
     *
     * @param firstHeaderPtr
     */
    void serFirstHeaderPointer(uint16_t firstHeaderPtr);

    /**
     * @brief Get the first header pointer from the data field status
     */
    uint16_t getFirstHeaderPointer() {
        return (((static_cast<uint8_t >(primaryHeader.at(4)) & 0x00FFU) << 8U) |
                static_cast<uint8_t >(primaryHeader.at(5))) & 0x07FFU;
    };

    /**
     * @brief Access the primary header
     * @details Access functions are used to avoid accidental misconfiguration of the returned object/value
     */
    String<PRIMARY_HEADER_SIZE> getPrimaryHeader() {
        return primaryHeader;
    };

    /**
     * @brief Access the secondary header
     *
     * @attention At the moment the secondary header part is not implemented, since it is not required.
     */
#if SECONDARY_HEADER_SIZE > 0U
    String<SECONDARY_HEADER_SIZE> getSecondaryHeader() {
        return secondaryHeader;
    };
#endif

    /**
     * @brief Get if there is a secondary header in the packet
     */
    bool secondaryHeaderExists() { return SECONDARY_HEADER_SIZE > 0U; }

    /**
     * @brief The status of the operational control field
     */
    bool ocfFlag() { return not operationalControlField.empty(); }

    /**
     * @brief Get the master channel ID from the primary header as defined in the standard
     */
    uint16_t masterChannelID() { return SPACECRAFT_IDENTIFIER & 0x03FFU; };

    /**
     * @brief Get the master channel frame count
     */
    uint8_t masterChannelFrameCount() { return mChannelFrameCount; };

    /**
     * @brief Get the running virtual channel frame count
     */
    uint8_t virtualChannelFrameCount() { return vChannelFrameCount; };

    /**
     * @brief Get the virtual channel ID contained in the packet
     */
    uint8_t virtualChannelID() { return virtChannelID; };
};

#endif // CCSDS_TM_PACKETS_CCSDSTRANSFERFRAME_HPP
