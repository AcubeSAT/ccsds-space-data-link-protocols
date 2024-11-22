#pragma once

#include <cstdint>

/**
 * A class that offers an interface for bidirectional communication
 * between the sending end of the TC Data Link and FOP-1. An abstract description
 * of the interface is presented in p.3 of CCSDS 232.1-B-2
 * (COMMUNICATIONS OPERATION PROCEDURE-1). For a detailed definition of all the signals,
 * @see p. 4 of mentioned document
 *
 */
class FrameOperationProcedureInterface {
    /**
     * All possible signal types involved with FOP-1 communication
     *
     * Higher Layers - FOP-1  communication
     * ===================================
     * DIRECTIVE_REQUEST: send a directive to FOP-1 from a higher layer
     * DIRECTIVE_NOTIFICATION: FOP-1's response to said directive
     * ASYNCHRONOUS_NOTIFICATION: FOP-1 notifies about an event
     *
     * Lower Layers - FOP-1 communication
     * =================================
     * REQUEST_TO_TRANSFER_FDU: FOP-1 asks for a frame transfer to lower layers
     * TRANSFER_NOTIFICATION: lower layer's response to said transfer request
     */
    enum SignalTypes {
        DIRECTIVE_REQUEST,
        DIRECTIVE_NOTIFICATION,
        ASYNCHRONOUS_NOTIFICATION,
        REQUEST_TO_TRANSFER_FDU,
        TRANSFER_NOTIFICATION
    };

    enum DirectiveRequestType {
        INITIATE_AD_SERVICE_NO_CLCW_CHECK,
        INITIATE_AD_SERVICE_WITH_CLCW_CHECK,
        INITIATE_AD_SERVICE_WITH_UNLOCK,
        INITIATE_AD_SERVICE_WITH_SET_VR,
        TERMINATE_AD_SERVICE,
        RESUME_AD_SERVICE,
        SET_NEW_VS_VALUE,
        SET_FOP_SLIDING_WINDOW_WIDTH,
        SET_T1_INITIAL,
        SET_TRANSMISSION_LIMIT,
        SET_TIMEOUT_TYPE
    };

    enum DirectiveNotificationType {
        ACCEPT_RESPONSE_TO_DIRECTIVE,
        REJECT_RESPONSE_TO_DIRECTIVE,
        POSITIVE_CONFIRM_RESPONSE_TO_DIRECTIVE,
        NEGATIVE_CONFIRM_RESPONSE_TO_DIRECTIVE
    };

    enum AsynchronousNotificationType {
        ALERT,
        SUSPEND
    };

    enum AsynchronousAlertNotificationQualifier {
        T1,
        LOCKOUT,
        SYNCH,
        NNR,
        CLCW,
        LLIF,
        TERM
    };

    enum TransferNotificationType {
        ACCEPT_RESPONSE_TO_REQUEST_TO_TRANSFER,
        REJECT_RESPONSE_TO_REQUEST_TO_TRANSFER,
        POSITIVE_CONFIRM_RESPONSE_TO_REQUEST_TO_TRANSFER,
        CONFIRM_CONFIRM_RESPONSE_TO_REQUEST_TO_TRANSFER
    };

    /**
     * In order to make it easier re-implementing this class for different platforms, every
     * signal will be represented internally as a uint32_t. The following format is used:
     *
     * Field  | signal type | signal contents |
     * Bits        0-2              3-31
     *
     * Where signal contents differ based on signal type:
     *
     * DIRECTIVE_REQUEST:
     *
     * DIRECTIVE_NOTIFICATION:
     *
     * ASYNCHRONOUS_NOTIFICATION:
     *
     * REQUEST_TO_TRANSFER_FDU:
     *
     * TRANSFER_NOTIFICATION:
     */
};

