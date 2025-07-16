/**
 * @file FramePrintingFunctions.hpp
 * @brief Functions for printing transfer frame fields in a nice format
 */

#pragma once
#include "TransferFrameTM.hpp"
#include "TransferFrameTC.hpp"
#include "CcsdsDefinitions.hpp"

namespace CCSDSDataLinkLayer {
    /**
     * @brief Auxiliary service that accepts TM transfer frames and prints their fields using the logger.
     *        Offered for debugging purposes.
     * @param key Virtual Channel + Master Channel Identifier
     * @param verbosePrimaryHeader Print the names of the primary header fields
     * @param verboseOCF Print the names of the ocf fields
     */
    static void printTransferFrameTM(Defs::VcidScidKey key,
                                     const TransferFrameTM &TransferFrameTM,
                                     bool verbosePrimaryHeader,
                                     bool verboseOCF);

    /**
     * @brief Auxiliary service that accepts TM transfer frames and print their fields. Offered for debugging purposes
     * @param verbosePrimaryHeader  Print the names of the primary header fields
     */
    static void printTransferFrameTC(Defs::VcidScidKey key,
                              const TransferFrameTC &TransferFrameTC,
                              bool verbosePrimaryHeader);
} // CCSDSDataLinkLayer
