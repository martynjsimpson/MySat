#include <Arduino.h>

namespace
{
  constexpr unsigned long kBaudRate = 115200;
  constexpr unsigned long kHeartbeatIntervalMs = 1000;

  unsigned long lastHeartbeatMs = 0;
  bool ledOn = false;
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(kBaudRate);
  while (!Serial)
  {
    // Wait for the USB serial port on boards that expose it.
  }

  Serial.println(F("MySat ground station firmware starting"));
}

void loop()
{
  const unsigned long now = millis();
  if ((now - lastHeartbeatMs) < kHeartbeatIntervalMs)
  {
    return;
  }

  lastHeartbeatMs = now;
  ledOn = !ledOn;
  digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);

  Serial.print(F("GROUND_STATION,HEARTBEAT_MS,"));
  Serial.println(now);
}
