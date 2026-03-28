#include "protocol.h"

#include <Arduino.h>
#include "protocol_internal.h"
#include "sender.h"

namespace
{
  const size_t BUFFER_SIZE = 96;
  char inputBuffer[BUFFER_SIZE];
  size_t inputPos = 0;
}

void readSerialCommands()
{
  while (Serial.available() > 0)
  {
    const char c = static_cast<char>(Serial.read());

    if (c == '\r')
    {
      continue;
    }

    if (c == '\n')
    {
      inputBuffer[inputPos] = '\0';

      if (inputPos > 0)
      {
        processCommand(inputBuffer);
      }

      inputPos = 0;
    }
    else
    {
      if (inputPos < BUFFER_SIZE - 1)
      {
        inputBuffer[inputPos++] = c;
      }
      else
      {
        inputBuffer[inputPos] = '\0';
        setErrorContext(inputBuffer);
        sendError("OVERFLOW");
        clearErrorContext();
        inputPos = 0;
      }
    }
  }
}

void setupProtocol()
{
  inputPos = 0;
}
