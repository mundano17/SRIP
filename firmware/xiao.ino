#include <ArduinoBLE.h>

// BLE UUIDs (MUST match Android exactly)
BLEService colorService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic rgbChar("19B10001-E8F2-537E-4F6C-D104768A1214", BLEWrite, 3);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ; // wait for Serial Monitor

  Serial.println("Starting BLE RGB Receiver...");

  if (!BLE.begin()) {
    Serial.println("❌ BLE init failed");
    while (1)
      ;
  }

  BLE.setLocalName("XIAO-Strap");
  BLE.setAdvertisedService(colorService);

  colorService.addCharacteristic(rgbChar);
  BLE.addService(colorService);

  BLE.advertise();
  Serial.println("✅ BLE advertising as XIAO-Strap");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("🔗 Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      if (rgbChar.written()) {
        const uint8_t *data = rgbChar.value();

        uint8_t r = data[0];
        uint8_t g = data[1];
        uint8_t b = data[2];

        Serial.print("🎨 RGB received → ");
        Serial.print("R=");
        Serial.print(r);
        Serial.print(" G=");
        Serial.print(g);
        Serial.print(" B=");
        Serial.println(b);
      }
    }

    Serial.println("❌ Disconnected");
  }
}
