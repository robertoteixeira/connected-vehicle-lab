#pragma once

#include <Arduino.h>
#include <time.h>

namespace AzureIoTAuth {
String urlEncode(const String& value);
String createSasToken(time_t expiresAt);
}  // namespace AzureIoTAuth