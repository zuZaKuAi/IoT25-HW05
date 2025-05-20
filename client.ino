#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED 설정
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// UUID 설정 (서버와 일치해야 함)
static BLEUUID serviceUUID("12345678-1234-1234-1234-1234567890ab");
static BLEUUID charUUID("abcd1234-ab12-cd34-ef56-abcdef123456");

static bool doConnect = false;
static bool connected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
static BLEAdvertisedDevice* myDevice = nullptr;
static BLEClient* pClient = nullptr;

// BLE 알림 콜백
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData, size_t length, bool isNotify) {

    String received = String((char*)pData).substring(0, length);
    Serial.print("BLE Received: ");
    Serial.println(received);

    static String line1 = "";
    static String line2 = "";

    if (received.startsWith("Temp")) {
        line1 = received;
    } else if (received.startsWith("Humidity")) {
        line2 = received;
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("BLE Data:");
    display.println(line1);
    display.println(line2);
    display.display();
}

// BLE 서버 연결 함수
bool connectToServer() {
    Serial.print("Connecting to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    pClient = BLEDevice::createClient();
    if (!pClient->connect(myDevice)) {
        Serial.println("Failed to connect");
        return false;
    }

    Serial.println(" - Connected to server");

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.println("Failed to find service");
        pClient->disconnect();
        return false;
    }

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("Failed to find characteristic");
        pClient->disconnect();
        return false;
    }

    if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

    connected = true;
    return true;
}

// BLE 스캔 콜백
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("Found device: ");
        Serial.println(advertisedDevice.toString().c_str());

        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Client...");

    // OLED 초기화
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Scanning BLE...");
    display.display();

    // BLE 초기화
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

void loop() {
    // 연결 시도
    if (doConnect) {
        if (connectToServer()) {
            Serial.println("BLE Connected");
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("BLE Connected!");
            display.display();

            doConnect = false;
            connected = true;
        } else {
            Serial.println("BLE Connect Failed");
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println("Connect Failed");
            display.display();

            delay(2000);  // 재시도 전 대기
        }
    }

    // 연결 끊김 감지
    if (connected && !pClient->isConnected()) {
        Serial.println("Connection lost. Reconnecting...");
        connected = false;
        doConnect = true;

        // 다시 스캔 시작
        BLEDevice::getScan()->start(5, false);
    }

    delay(1000);
}
