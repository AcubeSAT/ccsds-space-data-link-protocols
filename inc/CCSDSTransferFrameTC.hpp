#ifndef CCSDS_TRANSFERFRAMETC_HPP
#define CCSDS_TRANSFERFRAMETC_HPP

#include "CCSDS_Definitions.hpp"
#include "CCSDSTransferFrame.hpp"
#include "etl/String.hpp"

/**
 * @brief Define a CCSDS telecommand packet with the required attributes
*/
class CCSDSTransferFrameTC: private CCSDSTransferFrame {

private:
    /**
	 * @brief Hold the virtual channel ID included in the packet
	 */
    uint8_t virtChannelID;

    /**
     * @brief Hold the Frame Sequence Counter
     * @todo Look into on how this will be handled externally
     */
    uint8_t frameSeqCount;

    /**
     * @brief Hold the Frame Length
     */
    uint16_t frameLength;

    /**
     * @brief Holds the Bypass Flag
     */
    bool bypassFlag;

    /**
     * @brief Holds the Control Command Flag
     */
    bool ctrlCmdFlag;

    /**
     * @brief Internal function to create the packet's primary header upon the packet creation.
     * @details This particular function is called inside the constructor to generate the primary header of the packet.
     * The primary header is saved as the `String<TC_PRIMARY_HEADER_SIZE>` attribute in this class and can be accessed
     * publicly.
     *
     * @note As per CCSDS 232.0-B-3 recommendation, the transfer frame version number is `00`
    **/
    void createPrimaryHeader() override;

    /**
	 * @brief Hold the data field octets
	 * @details The length of this field is variable and it is determined by the mission requirements for the
	 * total packet length.
	 */
    String<TC_MAX_DATA_FIELD_SIZE> dataField;

protected:
#if TC_ERROR_CONTROL_FIELD_EXISTS
    /**
	 * @brief Store the result of the CRC generation for the packet as the standard recommends
	 *
	 * @attention This will probably not be implemented for our mission, since other error correcting and detection
     * techniques will be implemented
	 */
    String<2> errorControlField;
#endif

    /**
     * @brief Store the transfer frame primary header generated from the `createPrimaryHeader` function
    */
    String<TC_PRIMARY_HEADER_SIZE> primaryHeader;

public:

    explicit CCSDSTransferFrameTC(uint8_t vChannelID = 0, uint16_t frameLen = TC_MAX_TRANSFER_FRAME_SIZE,
            bool bypassFlag = false, bool ctrlCmdFlag = false)
    : primaryHeader(),
#if TC_ERROR_CONTROL_FIELD_EXISTS
    errorControlField(),
#endif
    frameLength(frameLen), frameSeqCount(0), virtChannelID(vChannelID), bypassFlag(bypassFlag),
    ctrlCmdFlag(ctrlCmdFlag) {
        createPrimaryHeader();
    }

    /**
     * @brief Sets Frame Sequence Counter
     */
    void setFrameSequenceCount(uint8_t count);

    /**
     * @brief Increment Frame Sequence Counter by one
     */
    void incrementFrameSequenceCount();

    /**
	 * @brief Get the generated transfer frame
     * @todo Handle variable length in higher level services
	 */
    String<TC_MAX_TRANSFER_FRAME_SIZE> transferFrame();

    /**
	 * @brief The total size of the transfer frame
	 */
    uint16_t getTransferFrameSize() override {
        return frameLength;
    };

    /**
	 * @brief Access the primary header
	 * @details Access functions are used to avoid accidental misconfiguration of the returned object/value
	 */
    String<TC_PRIMARY_HEADER_SIZE> getPrimaryHeader() {
        return primaryHeader;
    }
};

#endif // CCSDS_TRANSFERFRAMETC_HPP
