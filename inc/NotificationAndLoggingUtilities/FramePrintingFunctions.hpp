/**
 * @file FramePrintingFunctions.hpp
 * @brief Functions for printing transfer frame fields in a nice format
 */

#pragma once
#include "etl/expected.h"
#include "CcsdsDefinitions.hpp"
#include "TransferFrameTM.hpp"
#include "ChannelObjects.hpp"
#include "DataLinkNotifications.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_FRAME_PRINTING_FUNCTIONS
    class FramePrintingFunctions {
    public:
#if defined(INCLUDE_SPACE_SEGMENT_CODE)
        /**
         * @brief Debugging function that prints the fields of a TM frame using the logger
         * @param vcChanName Virtual channel the frame belong in.
         * @param verbosePrimaryHeader Print the names of the primary header fields
         * @param verboseSecondaryHeader Print the names of the secondary header fields
         * @param verboseSecurityHeader Print the names of the security header field
         * @param verboseOcfField Print the names of the ocf fields
         * @param ocfAppendFunc Function used to interpret the ocf field. Look  appendClcwField() as an example for
         *                      CLCWs
         * @note The function is compiled only if the INCLUDE_SPACE_SEGMENT_CODE is defined, as the Ss Tm segment has not
         *       been implemented yet
         */
        static etl::expected<void, ServiceChannelNotification> printTransferFrameTM(Objects::VirtualChannelTmName vcChanName,
                                                                                    const TransferFrameTM &transferFrameTM,
                                                                                    bool verbosePrimaryHeader,
                                                                                    bool verboseSecondaryHeader,
                                                                                    bool verboseSecurityHeader,
                                                                                    bool verboseOcfField,
                                                                                    void (*ocfAppendFunc)(bool, const uint8_t*) = appendClcwField);
#endif

        /**
        * @brief Debugging function that prints the fields of a TM frame using the logger
        * @param vcChanName Virtual channel the frame belong in
        * @param verbosePrimaryHeader  Print the names of the primary header fields
        * @param verboseSecurityHeader Print the names of the security header field
        */
        static etl::expected<void, ServiceChannelNotification> printTransferFrameTC(Objects::VirtualChannelTcName vcChanName,
                                  const TransferFrameTC &transferFrameTC,
                                  bool verbosePrimaryHeader,
                                  bool verboseSecurityHeader);

        /**
         * Accepts an ocf field and appends them in a string. The field is interpreted as
         * a clcw
         */
        static void appendClcwField(bool verboseOutput, const uint8_t* ocfFieldSrc);

    private:
        static constexpr uint16_t  TmPrintingFuncMaxMessageSize = Defs::MaxTmTransferFrameLength * 6 + 500;
        static constexpr uint16_t TcPrintingFuncMaxMessageSize = Defs::MaxTcTransferFrameLength * 6 + 330;
        static etl::string<TmPrintingFuncMaxMessageSize> tmDebugOutput;
        static etl::string<TcPrintingFuncMaxMessageSize> tcDebugOutput;
    };
#endif // INCLUDE_FRAME_PRINTING_FUNCTIONS
} // CCSDSDataLinkLayer
