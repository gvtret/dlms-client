#include "dlms/client/client_status.hpp"

namespace dlms {
namespace client {

const char* ClientStatusName(ClientStatus status)
{
  switch (status) {
  case ClientStatus::Ok:
    return "Ok";
  case ClientStatus::InvalidArgument:
    return "InvalidArgument";
  case ClientStatus::InvalidState:
    return "InvalidState";
  case ClientStatus::TransportOpenFailed:
    return "TransportOpenFailed";
  case ClientStatus::ChannelOpenFailed:
    return "ChannelOpenFailed";
  case ClientStatus::AssociationFailed:
    return "AssociationFailed";
  case ClientStatus::NotAssociated:
    return "NotAssociated";
  case ClientStatus::SendFailed:
    return "SendFailed";
  case ClientStatus::ReceiveFailed:
    return "ReceiveFailed";
  case ClientStatus::ServiceRejected:
    return "ServiceRejected";
  case ClientStatus::UnsupportedFeature:
    return "UnsupportedFeature";
  case ClientStatus::InternalError:
    return "InternalError";
  }

  return "Unknown";
}

} // namespace client
} // namespace dlms
