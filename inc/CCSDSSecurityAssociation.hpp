#pragma once

/**
 * The Security Association (SA) is an entity defined within the Space Data Link Security Protocol
 * (CCSDS 355.0-B-2) and is responsible for offering authentication and encryption capabilities for
 * the Data Link Layer. This specific implementation:
 *  - is static, meaning that SAs will not be created and destroyed for the dureation of the mission
 *  - offers only authentication capabilities (40-bit HMAC). It is built in way that allows easy addition of
 *    encryption and authentication-encryption, should that be necessary.
 *  - supports only the TC Data Link Protocol
 *  - is built as a bidirectional interface. This means that it stores the necessary parameters for both ends.
 *    the sending end user is meant to call the 'applySecurity' function, while the receiving end user is meant to
 *    call the 'ProcessSecurity' function.
*/