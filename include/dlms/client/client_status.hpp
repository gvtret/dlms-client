#pragma once

namespace dlms {
namespace client {

enum class ClientStatus
{
  Ok,
  InvalidArgument,
  InvalidState,
  TransportOpenFailed,
  ChannelOpenFailed,
  AssociationFailed,
  NotAssociated,
  SendFailed,
  ReceiveFailed,
  ServiceRejected,
  SecurityFailed,
  UnsupportedFeature,
  InternalError
};

const char* ClientStatusName(ClientStatus status);

} // namespace client
} // namespace dlms
