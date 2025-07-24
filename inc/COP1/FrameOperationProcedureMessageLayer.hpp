/**
 * @file FrameOperationProcedureMessageLayer.hpp
 *
 * @brief This file defines signals to streamline interfacing with FOP-1 and increase modularity
 *
 */

#pragma once
#include "TransferFrameTC.hpp"

namespace CCSDSDataLinkLayer {
#ifdef INCLUDE_GROUND_SEGMENT_CODE
    /**
     * Directive request signal
     * @see p. 3.2.2.2.2 & 4.1 from COP-1 CCSDS
     */
    enum class DirectiveRequestType : uint8_t {
        INITIATE_AD_SERVICE_WITHOUT_CLCW_CHECK = 1,
        INITIATE_AD_SERVICE_WITH_CLCW_CHECK = 2,
        INITIATE_AD_SERVICE_WITH_UNLOCK = 3,
        INITIATE_AD_SERVICE_WITH_SET_VR = 4,
        TERMINATE_AD_SERVICE = 5,
        RESUME_AD_SERVICE = 6,
        SET_NEW_VS = 7,
        SET_FOP_SLIDING_WINDOW_WIDTH = 8,
        SET_T1_INITIAL = 9,
        SET_TRANSMISSION_LIMIT = 10,
        SET_TIMEOUT_TYPE
    };

    struct DirectiveRequestSignal {
        uint8_t requestIdentifier;
        DirectiveRequestType directiveType;
        etl::optional<uint16_t> directiveQualifier;

        DirectiveRequestSignal(const uint8_t requestIdentifier, const DirectiveRequestType directiveType,
                               const etl::optional<uint16_t> &directiveQualifier = etl::nullopt) : requestIdentifier(
                requestIdentifier), directiveType(directiveType),
            directiveQualifier(directiveQualifier) {
        }
    };

    /**
     * Directive notification signal
     * @see p. 3.2.2.2.3 & 4.2 from COP-1 CCSDS
     */
    enum class DirectiveNotificationType : uint8_t {
        ACCEPT_RESPONSE_TO_DIRECTIVE,
        REJECT_RESPONSE_TO_DIRECTIVE,
        POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE,
        NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE
    };

    struct DirectiveNotificationSignal {
        uint8_t requestIdentifier;
        DirectiveNotificationType directiveNotificationType;
        etl::optional<TransferFrameTC *> frame;

        DirectiveNotificationSignal(const uint8_t requestIdentifier,
                                    const DirectiveNotificationType directiveNotificationType,
                                    const etl::optional<TransferFrameTC *> &frame = etl::nullopt) : requestIdentifier(
                requestIdentifier), directiveNotificationType(directiveNotificationType),
            frame(frame) {
        };
    };

    // A user version of the directive notification signal
    struct DirectiveNotificationSignalUser {
        uint8_t requestIdentifier;
        DirectiveNotificationType directiveNotificationType;

        DirectiveNotificationSignalUser(const uint8_t requestIdentifier,
                                    const DirectiveNotificationType directiveNotificationType) : requestIdentifier(
                requestIdentifier), directiveNotificationType(directiveNotificationType) {}
    };

    /**
     * Asynchronous notification signal
     * @see p. 3.2.2.2.4 & 4.3 from COP-1 CCSDS
     */
    enum class AsynchronousNotificationType : uint8_t {
        ALERT,
        SUSPEND
    };

    enum class AlertEvent : uint8_t {
        ALRT_SYNCH = 0,
        ALRT_CLCW = 1,
        ALRT_LIMIT = 2,
        ALRT_TERM = 3,
        ALRT_LLIF = 4,
        ALRT_NNR = 5,
        ALRT_LOCKOUT = 6,
        ALRT_T1 = 7,
        ALRT_NONE = 8
    };

    struct AsynchronousNotificationSignal {
        AsynchronousNotificationType asynchronousNotificationType;
        etl::optional<AlertEvent> alertEvent;

        explicit AsynchronousNotificationSignal(const AsynchronousNotificationType asynchronousNotificationType,
                                                const etl::optional<AlertEvent> &alertEvent =
                                                        etl::nullopt) : asynchronousNotificationType(
                                                                            asynchronousNotificationType),
                                                                        alertEvent(alertEvent) {
        }
    };

    /**
     * FDU Transfer signal
     * @see p. 3.2.2.3 from COP-1 CCSDS
     */
    struct FduTransferSignal {
        TransferFrameTC *frame;

        FduTransferSignal(TransferFrameTC *frame) : frame(frame) {}
    };

    /**
     * Transfer notification signal
     * @see p. 3.2.2.3.3 & 4.4 from COP-1 CCSDS
     */
    enum class TransferNotificationType : uint8_t {
        ACCEPT_RESPONSE_TO_TRANSFER_FDU, // for AD & BD frames
        REJECT_RESPONSE_TO_TRANSFER_FDU, // for AD & BD frames
        POSITIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU, // for AD frames only
        NEGATIVE_CONFIRM_RESPONSE_TO_TRANSFER_FDU // for AD frames only
    };

    struct TransferNotificationSignal {
        TransferNotificationType transferNotificationType;
        etl::optional<TransferFrameTC *> frame;

        explicit TransferNotificationSignal(const TransferNotificationType transferNotificationType,
                                            const etl::optional<TransferFrameTC *> &frame =
                                                    etl::nullopt) : transferNotificationType(transferNotificationType),
                                                                    frame(frame) {
        }
    };


    /**
     *  Transmit & abort request for frames signal
     *  @see p. 3.2.3 from COP-1 CCSDS
     */
    enum class LowerLayerRequestType : bool {
        LOW_LAYER_TRANSMIT,
        LOW_LAYER_ABORT // lower layers should abort all AD (or BC) frame transmission
    };

    struct FopToLowerLayerRequestSignal {
        LowerLayerRequestType lowerLayerRequestType;
        etl::optional<TransferFrameTC *> frame;

        explicit FopToLowerLayerRequestSignal(const LowerLayerRequestType lowerLayerRequestType,
            const etl::optional<TransferFrameTC *> &frame =etl::nullopt)
        : lowerLayerRequestType(lowerLayerRequestType), frame(frame) {
        }
    };

    /**
     *  Transmit request response signal
     *  @see p. 3.2.3 from COP-1 CCSDS
     */
    enum class LowerLayerResponseSignal : uint8_t {
        AD_ACCEPT,
        AD_REJECT,
        BC_ACCEPT,
        BC_REJECT,
        BD_ACCEPT,
        BD_REJECT
    };
#endif // INCLUDE_GROUND_SEGMENT_CODE
} // CCSDS_DATA_LINK_LAYER