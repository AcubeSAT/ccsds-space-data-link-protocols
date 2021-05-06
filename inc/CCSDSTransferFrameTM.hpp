#ifndef CCSDS_PACKETS_TRANSFERFRAMETM_HPP
#define CCSDS_PACKETS_TRANSFERFRAMETM_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrame.hpp"
#include "etl/String.hpp"

/**
 * @brief Define a CCSDS packet with the required attributes
 *
 * @todo Define if the Operational Control Field and the Secondary header are required for our project
 */
class CCSDSTransferFrameTM : private CCSDSTransferFrame {
    // Class variable declarations
private:
    /**
     * @brief Hold the running count of the master channel frame count
     * @todo Decide on how master channels will be handled
     */
    uint8_t mChannelFrameCount;

    /**
     * @brief Hold the running count of the virtual channel frame count
     */
    uint8_t vChannelFrameCount;

    /**
     * @brief Hold the virtual channel ID included in the packet
     */
    uint8_t virtChannelID;

    /**
     * @brief Indicate the overflow status of the master channel frame count
     */
    bool masterChannelOverflowFlag;

    /**
     * @brief Indicate the overflow status of the virtual channel frame count
     */
    bool virtualChannelOverflowFlag;

    /**
     * @brief Defines whether secondary header is present
     */
    bool secondaryHeaderPresent;

protected:
    /**
     * @brief Store the operational control field octets
     *
     * @attention For the moment the operational control field is not implemented and the usage is TBD
     */
    String<4> operationalControlField;

#if tm_error_control_field_exists
    /**
     * @brief Store the result of the CRC generation for the packet as the standard recommends
     *
     * @attention This will not be implemented for our mission, since other error correcting and detection techniques
     * will be implemented
     */
    String<2> errorControlField;
#endif

    /**
     * @brief Store the transfer frame primary header generated from the `createPrimaryHeader` function
     */
    String<tm_primary_header_size> primaryHeader;

    /**
     * @brief Store the transfer frame secondary header generated from the `createSecondaryHeader` function
     *
     * @attention At the moment the secondary header part is not implemented, since it is not required.
     */
#if tm_secondary_header_size > 0U
    String<tm_secondary_header_size> secondaryHeader;
#endif

public:
    /**
     * @brief Hold the data field octets
     * @details The length of this field is variable and it is determined by the mission requirements for the
     * total packet length.
     */
    String<tm_frame_data_field_size> dataField;

private:
    /**
     * @brief Internal function to create the packet's primary header upon the packet creation.
     * @details This particular function is called inside the constructor to generate the primary header of the packet.
     * The primary header is saved as the `String<tm_primary_header_size>` attribute in this class and can be accessed
     * publicly.
     *
     * @note As per CCSDS 132.0-B-2 recommendation, the transfer frame version number is `00`
     * @attention The synchronization flag and the packet order flag of the data field status, in the primary header,
     * are assumed to be `0` in this implementation, thus requiring that the segment length ID is `11`.
     */
    void createPrimaryHeader() override;

    /**
     * @brief Generates the transfer frame secondary header
     *
     * @attention At the moment the secondary header part is not implemented, since it is not required.
     */
#if tm_secondary_header_size > 0U
    void createSecondaryHeader(String<tm_secondary_header_size - 1>& dataField) override;
#endif

public:
    /**
     *
     *
     * @param datFieldSize Provide the total length in octets of the data field
     * @param virtChannelID Provide the virtual channel ID, if used, otherwise it is zero by default
     */
    explicit CCSDSTransferFrameTM(uint8_t vChannelID = 0) // Ignore-MISRA
            : primaryHeader(),
#if tm_secondary_header_size > 0U
            secondaryHeader(),
#endif
              secondaryHeaderPresent(tm_secondary_header_size > 0), dataField(), operationalControlField(),
#if tm_error_control_field_exists
            errorControlField(),
#endif
              mChannelFrameCount(0), vChannelFrameCount(0), virtChannelID(vChannelID),
              masterChannelOverflowFlag(false), virtualChannelOverflowFlag(false) {
        createPrimaryHeader();
    }

    /**
     * @brief Increase the master channel frame count
     *
     * @details The master channel frame count field provides the running count of the frames transmitted through the
     * same master channel. Since the field is 8-bit, care should be taken to avoid overflow and also as per the
     * standard's recommendation, re-setting of the count before reaching the value of 255 should be avoided.
     */
    void increaseMasterChannelFrameCount();

    /**
     * @brief Rest the running count for the master channel
     */
    void resetMasterChannelFrameCount() {
        mChannelFrameCount = 0U;
    };

    /**
     * @brief Increase the virtual channel frame count
     * @details The virtual channel frame count field provides the individual accountability for each virtual channel,
     * to enable systematic packet extraction from the transfer frame data field. Same as the master count, re-setting
     * before 255 should be avoided and the count is again 8-bit.
     */
    void increaseVirtualChannelFrameCount();

    /**
     * @brief Rest the running count for the virtual channel this frame contains
     */
    void resetVirtualChannelFrameCount() {
        vChannelFrameCount = 0U;
    };

    /**
     * @brief Set the 11-bit first header pointer in the data field status of the primary header
     * @details
     *
     * @param firstHeaderPtr
     */
    void setFirstHeaderPointer(uint16_t firstHeaderPtr);

    /**
     * @brief Get the first header pointer from the data field status
     */
    uint16_t getFirstHeaderPointer() {
        return (((static_cast<uint8_t>(primaryHeader.at(4)) & 0x00FFU) << 8U) |
                static_cast<uint8_t>(primaryHeader.at(5))) &
               0x07FFU;
    }

    /**
     * @brief Access the primary header
     * @details Access functions are used to avoid accidental misconfiguration of the returned object/value
     */
    String<tm_primary_header_size> getPrimaryHeader() {
        return primaryHeader;
    }

    /**
     * @brief Access the secondary header
     *
     * @attention At the moment the secondary header part is not implemented, since it is not required.
     */
#if tm_secondary_header_size > 0U
    String<tm_secondary_header_size> getSecondaryHeader() {
        return secondaryHeader;
    };
#endif

    /**
     * @brief Get if there is a secondary header in the packet
     */
    bool secondaryHeaderExists() {
        return tm_secondary_header_size > 0U;
    }

    /**
     * @brief Check whether the master channel frame count has been reset
     *
     * @todo Define if this function is needed
     */
    bool masterFrameCountOverflowStatus() {
        return masterChannelOverflowFlag;
    }

    /**
     * @brief Check whether the virtual channel frame count has been reset
     *
     * @todo Define if this function is needed
     */
    bool virtualFrameCountOverflowStatus() {
        return virtualChannelOverflowFlag;
    }

    /**
     * @brief The status of the operational control field
     */
    bool ocfFlag() {
        return not operationalControlField.empty();
    }

    /**
     * @brief Get the master channel ID from the primary header as defined in the standard
     */
    uint16_t masterChannelID() {
        return spacecraft_identifier & 0x03FFU;
    };

    /**
     * @brief Get the master channel frame count
     */
    uint8_t masterChannelFrameCount() {
        return mChannelFrameCount;
    };

    /**
     * @brief Get the running virtual channel frame count
     */
    uint8_t virtualChannelFrameCount() {
        return vChannelFrameCount;
    };

    /**
     * @brief Get the virtual channel ID contained in the packet
     */
    uint8_t virtualChannelID() {
        return virtChannelID;
    };

    /**
     * @brief Get the generated transfer frame
     */
    String<tm_transfer_frame_size> transferFrame();

    /**
     * @brief The total size of the transfer frame
     */
    uint16_t getTransferFrameSize() override;
};

#endif // CCSDS_PACKETS_TRANSFERFRAMETM_HPP
