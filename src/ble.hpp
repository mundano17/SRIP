#pragma once

#include <cstdint>

#include <simpleble/Adapter.h>
#include <simpleble/Peripheral.h>

class BLEColorClient {
public:
  const char *COLOR_SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214";

  const char *COLOR_CHARACTERISTIC_UUID =
      "19B10001-E8F2-537E-4F6C-D104768A1214";

  BLEColorClient() = default;
  ~BLEColorClient();

  bool initialize();
  bool connect();
  bool connected();

  void sendColor(uint8_t r, uint8_t g, uint8_t b);

private:
  SimpleBLE::Adapter adapter;
  SimpleBLE::Peripheral peripheral;
};
