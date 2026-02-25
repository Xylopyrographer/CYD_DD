#pragma once
#include <Arduino.h>

// XOR+hex obfuscation for WiFi passwords in NVS.
// Not cryptographic — intended as casual obfuscation only.
// Backward compatible: non-hex or odd-length values are treated as legacy plaintext.
String obfuscatePassword( const String &plain );
String deobfuscatePassword( const String &stored );
