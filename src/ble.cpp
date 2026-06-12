#include "ble.hpp"
#include "simpleble/Adapter.h"
#include "simpleble/Peripheral.h"
#include <iostream>
#include <ostream>
#include <simpleble/SimpleBLE.h>

bool BLEColorClient::initialize() {
  if (!SimpleBLE::Adapter::bluetooth_enabled()) {
    std::cerr << "Bluetooth is not enabled or permission has not been granted."
              << std::endl;
    return false;
  }

  std::cout << "Bluetooth is available." << std::endl;
  return true;
};

bool BLEColorClient::connect() {
  auto adapters = SimpleBLE::Adapter::get_adapters();
  if (adapters.empty()) {
    throw std::runtime_error("No Bluetooth adapter found");
  }
  adapter = adapters[0];
  adapter.scan_for(5000);
  auto peripherals = adapter.scan_get_results();
  for (auto &p : peripherals) {
    try {
      std::cout << p.identifier() << std::endl;
      if (p.identifier() != "XIAO-Strap")
        continue;
      p.connect();
      for (auto &service : p.services()) {
        if (service.uuid() != COLOR_SERVICE_UUID)
          continue;
        for (auto &characteristic : service.characteristics()) {
          if (characteristic.uuid() == COLOR_CHARACTERISTIC_UUID &&
                  characteristic.can_write_command() ||
              characteristic.can_write_request()) {
            peripheral = p;
            return true;
          }
        }
      }
      p.disconnect();
    } catch (const std::exception &e) {
      std::cerr << "[BLE] " << p.identifier() << ": " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "[BLE] " << p.identifier() << ": Unknown error" << std::endl;
    }
  }
  return false;
}

bool BLEColorClient::connected() { return peripheral.is_connected(); };

void BLEColorClient::sendColor(uint8_t r, uint8_t g, uint8_t b) {
  try {
    if (!peripheral.is_connected())
      return;
    std::vector<uint8_t> packet = {r, g, b};
    peripheral.write_request(COLOR_SERVICE_UUID, COLOR_CHARACTERISTIC_UUID,
                             packet);
  } catch (const std::exception &e) {
    std::cerr << "[BLE Write] " << e.what() << std::endl;
  }
}

BLEColorClient::~BLEColorClient() {
  if (peripheral.is_connected())
    peripheral.disconnect();
}
