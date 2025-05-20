#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// BLE UUID 설정
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-ab12-cd34-ef56-abcdef123456"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client connected.");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client disconnected. Restarting advertising...");
      BLEDevice::startAdvertising(); // 재광고
    }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32_BLE_Server");

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
                    );

  pCharacteristic->setValue("Hello BLE");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  Serial.println("BLE Server Started and Advertising");
}

void loop() {
  // 간단히 notify로 신호 유지용 전송
  if (deviceConnected) {
    pCharacteristic->setValue("Ping");
    pCharacteristic->notify();
  }
  delay(2000);
}
