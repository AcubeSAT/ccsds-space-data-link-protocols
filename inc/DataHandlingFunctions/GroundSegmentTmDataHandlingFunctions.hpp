/**
 * @file GroundSegmentTmDataHandlingFunctions.hpp
 * @brief Functions for receiving and processing TM Transfer Frames
 */

#pragma once

#ifdef INCLUDE_GROUND_SEGMENT_CODE
namespace CCSDSDataLinkLayer {
    class GroundSegmentTmServices;

    class GsTmDataHandling {
        friend class GroundSegmentTmServices;
    };
} // CCSDSDataLinkLayer
#endif // INCLUDE_GROUND_SEGMENT_CODE