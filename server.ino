#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "DHT.h"

#define DHTPIN 4       // DHT11 센서가 연결된 핀 번호
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// BLE UUID
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

      // 클라이언트가 끊기면 다시 광고 시작
      BLEDevice::startAdvertising();
    }
};


void setup() {
  Serial.begin(115200);
  dht.begin();

  BLEDevice::init("ESP32_DHT11_Server");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setValue("Starting...");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  Serial.println("BLE Server Started and Advertising");
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // 섭씨

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  char tempPayload[25];
char humidPayload[25];
snprintf(tempPayload, sizeof(tempPayload), "Temp: %.1f*C", t);
snprintf(humidPayload, sizeof(humidPayload), "Humidity: %.1f%%", h);

Serial.println(tempPayload);
Serial.println(humidPayload);

if (deviceConnected) {
  pCharacteristic->setValue((uint8_t*)tempPayload, strlen(tempPayload));
  pCharacteristic->notify();
  delay(100);  // 너무 빠르게 두 번 전송하지 않도록 약간의 딜레이

  pCharacteristic->setValue((uint8_t*)humidPayload, strlen(humidPayload));
  pCharacteristic->notify();
}


  delay(2000); // 2초마다 전송
}
