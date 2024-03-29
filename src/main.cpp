#include <M5stickC.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "imu/ImuReader.h"
#include "imu/AverageCalc.h"
#include "input/ButtonCheck.h"
#include "input/ButtonData.h"
#include "session/SessionData.h"
#include "prefs/Settings.h"

// wifi
#define SEND_DATA_NUM 4
#define SSID ""
#define PASSWORD ""
#define CLIENT_ADDRESS ""  // for send
#define CLIENT_PORT 22222  // for send

// tasks
#define TASK_DEFAULT_CORE_ID 1
#define TASK_STACK_DEPTH 4096UL
#define TASK_NAME_IMU "IMUTask"
#define TASK_NAME_WRITE_SESSION "WriteSessionTask"
#define TASK_NAME_BUTTON "ButtonTask"
#define TASK_SLEEP_IMU 5             // = 1000[ms] / 200[Hz]
#define TASK_SLEEP_WRITE_SESSION 5   // = 1000[ms] / 200[Hz]
#define TASK_SLEEP_BUTTON 1          // = 1000[ms] / 1000[Hz]
#define MUTEX_DEFAULT_WAIT 1000UL

void initM5LCD();
void initGyro();
void initWifi();
static void ImuLoop(void* arg);
static void WriteSessionLoop(void* arg);
static void ReadSessionLoop(void* arg);
static void ButtonLoop(void* arg);

imu::ImuReader* imuReader;
WiFiUDP udp;
input::ButtonCheck button;

imu::ImuData imuData;
input::ButtonData btnData;
bool hasButtonUpdate = false;
static SemaphoreHandle_t imuDataMutex = NULL;
static SemaphoreHandle_t btnDataMutex = NULL;

bool gyroOffsetInstalled = true;
imu::AverageCalcXYZ gyroAve;
prefs::Settings settingPref;

void setup() {
    M5.begin();
    Serial.begin(115200);

    initM5LCD();

    M5.Lcd.print("-- ryap --");

    initGyro();
    initWifi();

    M5.Lcd.println(WiFi.localIP());

    imuDataMutex = xSemaphoreCreateMutex();
    btnDataMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(ImuLoop, TASK_NAME_IMU, TASK_STACK_DEPTH, NULL, 2,
                            NULL, TASK_DEFAULT_CORE_ID);
    xTaskCreatePinnedToCore(WriteSessionLoop, TASK_NAME_WRITE_SESSION,
                            TASK_STACK_DEPTH, NULL, 1, NULL,
                            TASK_DEFAULT_CORE_ID);
    xTaskCreatePinnedToCore(ButtonLoop, TASK_NAME_BUTTON, TASK_STACK_DEPTH,
                            NULL, 1, NULL, TASK_DEFAULT_CORE_ID);
}

void loop() {
    // nothing to do
}

void initM5LCD() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(2, 0);
}

void initGyro() {
    float gyroOffset[3] = {0.0F};
    settingPref.begin();
    settingPref.readGyroOffset(gyroOffset);
    settingPref.finish();

    imuReader = new imu::ImuReader(M5.Imu);
    imuReader->initialize();
    if (gyroOffsetInstalled) {
        imuReader->writeGyroOffset(gyroOffset[0], gyroOffset[1], gyroOffset[2]);
    }
}

void initWifi() {
    M5.Mpu6886.Init();
    Serial.println("[ESP32] Connecting to WiFi network: " + String(SSID));
    WiFi.disconnect(true, true);
    delay(500);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

static void ImuLoop(void* arg) {
    while (1) {
        uint32_t entryTime = millis();
        if (xSemaphoreTake(imuDataMutex, MUTEX_DEFAULT_WAIT) == pdTRUE) {
            imuReader->update();
            imuReader->read(imuData);
            if (!gyroOffsetInstalled) {
                if (!gyroAve.push(imuData.gyro[0], imuData.gyro[1],
                                  imuData.gyro[2])) {
                    float x = gyroAve.averageX();
                    float y = gyroAve.averageY();
                    float z = gyroAve.averageZ();
                    // set offset
                    imuReader->writeGyroOffset(x, y, z);
                    // save offset
                    float offset[] = {x, y, z};
                    settingPref.begin();
                    settingPref.writeGyroOffset(offset);
                    settingPref.finish();
                    gyroOffsetInstalled = true;
                    gyroAve.reset();
                    // UpdateLcd();
                }
            }
        }
        xSemaphoreGive(imuDataMutex);
        // idle
        int32_t sleep = TASK_SLEEP_IMU - (millis() - entryTime);
        vTaskDelay((sleep > 0) ? sleep : 0);
    }
}

static void WriteSessionLoop(void* arg) {
    static session::SessionData imuSessionData(session::DataDefineImu);
    static session::SessionData btnSessionData(session::DataDefineButton);
    while (1) {
        uint32_t entryTime = millis();
        // imu
        if (gyroOffsetInstalled) {
            if (xSemaphoreTake(imuDataMutex, MUTEX_DEFAULT_WAIT) == pdTRUE) {
                udp.beginPacket(CLIENT_ADDRESS, CLIENT_PORT);
                imuSessionData.write((uint8_t*)&imuData, imu::ImuDataLen);
                udp.write((uint8_t*)&imuSessionData, imuSessionData.length());
                udp.endPacket();
            }
            xSemaphoreGive(imuDataMutex);
        }
        // button
        if (xSemaphoreTake(btnDataMutex, MUTEX_DEFAULT_WAIT) == pdTRUE) {
            if (hasButtonUpdate) {
                udp.beginPacket(CLIENT_ADDRESS, CLIENT_PORT);
                btnSessionData.write((uint8_t*)&btnData, input::ButtonDataLen);
                udp.write((uint8_t*)&btnSessionData, btnSessionData.length());
                udp.endPacket();
                hasButtonUpdate = false;
            }
            xSemaphoreGive(btnDataMutex);
        }
        // idle
        int32_t sleep = TASK_SLEEP_WRITE_SESSION - (millis() - entryTime);
        vTaskDelay((sleep > 0) ? sleep : 0);
    }
}

static void ButtonLoop(void* arg) {
    uint8_t btnFlag = 0;
    while (1) {
        uint32_t entryTime = millis();
        M5.update();
        if (button.containsUpdate(M5, btnFlag)) {
            for (int i = 0; i < INPUT_BTN_NUM; i++) {
                if (xSemaphoreTake(btnDataMutex, MUTEX_DEFAULT_WAIT) ==
                    pdTRUE) {
                    btnData.timestamp = millis();
                    btnData.btnBits = btnFlag;
                    hasButtonUpdate = true;
                }
                xSemaphoreGive(btnDataMutex);
            }
        }
        // idle
        int32_t sleep = TASK_SLEEP_BUTTON - (millis() - entryTime);
        vTaskDelay((sleep > 0) ? sleep : 0);
    }
}
