/**
 * @file SecurityAssociationServices.hpp
 * @brief Functions for controlling security associations
 */

#pragma once
#include <etl/expected.h>
#include "DataLinkNotifications.hpp"
#include "CcsdsDefinitions.hpp"
#include "ChannelObjects.hpp"

namespace CCSDSDataLinkLayer {
    class SecurityAssociationServices {
    public:
        /**
         * @brief Reset the authentication sequence counter of a specified security association
         * @param spi: Security parameter index of security association
         *
         * @note This function can be used to reset the sequence number automatically per spacecraft pass (or another
         *       specified interval), to ensure that it does not overflow. It can also be used to
         *       recover from a situation where the frame sequence number exceeded the sequence number window
         */
        static etl::expected<void, ServiceChannelNotification> resetSequenceNumber(Defs::Spi spi);

        /**
         * @brief Pause/Continue a security association. The effect of a pause is that channels associated with this SA
         *        will not be subject to security processing (although the security headers and trailers still exist)
         *
         * @note Both sender and receiver need to pause their respective SAs for communication to continue. The
         *       synchronization can be achieved by the sender sending am application layer command to the receiver.
         *       The received could also automatically call the function in dire operational circumstances.
         */
        static etl::expected<void, ServiceChannelNotification> pauseSecurityAssociation(Defs::Spi spi);
        static etl::expected<void, ServiceChannelNotification> continueSecurityAssociation(Defs::Spi spi);
    };
}
