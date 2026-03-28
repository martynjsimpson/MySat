#include "status.h"

#include "sender.h"

namespace
{
  unsigned long heartbeatCount = 0;
}

void setupStatus()
{
  heartbeatCount = 0;
}

void reportStatusStarted()
{
  sendTelemetry("STATUS", "STARTED", "TRUE");
}

void reportStatusHeartbeat(bool incrementHeartbeat)
{
  if (incrementHeartbeat)
  {
    ++heartbeatCount;
  }

  sendTelemetryULong("STATUS", "HEARTBEAT_N", heartbeatCount);
}
