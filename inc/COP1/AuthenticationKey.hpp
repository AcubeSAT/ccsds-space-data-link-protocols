#include "etl/string.h"
#include "CcsdsDefinitions.hpp"

namespace CCSDSDataLinkLayer {
    // This key is used for frame authentication, if the relevant service is enabled. Do not make it public
    inline const etl::string<Defs::MaxAuthenticationKeyLength> AuthenticationKey = etl::string<Defs::MaxAuthenticationKeyLength>{"1063324d5ea012e6"};
}