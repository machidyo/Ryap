#include <M5stickC.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "imu/ImuReader.h"
#include "imu/AverageCalc.h"
#include "prefs/Settings.h"

// for wifi
#define SEND_DATA_NUM 4
#define SSID ""
#define PASSWORD ""
#define CLIENT_ADDRESS "192.168.137.1"  // for send
#define CLIENT_PORT 22222               // for send

// for imu task
#define TASK_DEFAULT_CORE_ID 1
#define TASK_STACK_DEPTH 4096UL
#define TASK_NAME_IMU "IMUTask"
#define TASK_NAME_WRITE_SESSION "WriteSessionTask"
#define TASK_SLEEP_IMU 5             // = 1000[ms] / 200[Hz]
#define TASK_SLEEP_WRITE_SESSION 40  // = 1000[ms] / 25[Hz]
#define MUTEX_DEFAULT_WAIT 1000UL

void initM5LCD();
void initGyro();
void initWifi();
static void ImuLoop(void *arg);
static void WriteSessionLoop(void *arg);

imu::ImuReader *imuReader;
imu::ImuData imuData;
static SemaphoreHandle_t imuDataMutex = NULL;

WiFiUDP udp;

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
    xTaskCreatePinnedToCore(ImuLoop, TASK_NAME_IMU, TASK_STACK_DEPTH, NULL, 2,
                            NULL, TASK_DEFAULT_CORE_ID);
    xTaskCreatePinnedToCore(WriteSessionLoop, TASK_NAME_WRITE_SESSION,
                            TASK_STACK_DEPTH, NULL, 1, NULL,
                            TASK_DEFAULT_CORE_ID);
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

static void ImuLoop(void *arg) {
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

static void WriteSessionLoop(void *arg) {
    while (1) {
        uint32_t entryTime = millis();
        if (gyroOffsetInstalled) {
            if (xSemaphoreTake(imuDataMutex, MUTEX_DEFAULT_WAIT) == pdTRUE) {
                udp.beginPacket(CLIENT_ADDRESS, CLIENT_PORT);
                for (int i = 0; i < SEND_DATA_NUM; i++) {
                    byte data[sizeof(float)];
                    memcpy(data, &imuData.quat[i], sizeof(imuData.quat[i]));
                    udp.write(data, sizeof(data));
                }
                udp.endPacket();
            }
            xSemaphoreGive(imuDataMutex);
        }
        // idle
        int32_t sleep = TASK_SLEEP_WRITE_SESSION - (millis() - entryTime);
        vTaskDelay((sleep > 0) ? sleep : 0);
    }
}
