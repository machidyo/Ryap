#include <M5stickC.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "imu/ImuReader.h"
#include "imu/AverageCalc.h"
#include "prefs/Settings.h"

#define TASK_DEFAULT_CORE_ID 1
#define TASK_STACK_DEPTH 4096UL
#define TASK_NAME_IMU "IMUTask"
#define TASK_SLEEP_IMU 5  // = 1000[ms] / 200[Hz]
#define MUTEX_DEFAULT_WAIT 1000UL

// start debug memo network
#define SENDDATANUM 4

WiFiUDP udp;
float roll, pitch, yaw;

typedef union {
    int32_t ival;
    float fval;
    byte binary[4];
} uf;
uf s_ufdata[SENDDATANUM];
// end   debug memo network

static void ImuLoop(void *arg);

imu::ImuReader *imuReader;
imu::ImuData imuData;
static SemaphoreHandle_t imuDataMutex = NULL;

bool gyroOffsetInstalled = true;
imu::AverageCalcXYZ gyroAve;
prefs::Settings settingPref;

void InitM5LCD() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(2, 0);
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    InitM5LCD();

    M5.Lcd.print("-- ryap --");

    float gyroOffset[3] = {0.0F};
    settingPref.begin();
    settingPref.readGyroOffset(gyroOffset);
    settingPref.finish();

    // imu
    imuReader = new imu::ImuReader(M5.Imu);
    imuReader->initialize();
    if (gyroOffsetInstalled) {
        imuReader->writeGyroOffset(gyroOffset[0], gyroOffset[1], gyroOffset[2]);
    }

    // WiFi
    M5.Mpu6886.Init();
    Serial.println("[ESP32] Connecting to WiFi network: " + String(ssid));
    WiFi.disconnect(true, true);
    delay(500);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    M5.Lcd.println(WiFi.localIP());

    // task
    imuDataMutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(ImuLoop, TASK_NAME_IMU, TASK_STACK_DEPTH, NULL, 2,
                            NULL, TASK_DEFAULT_CORE_ID);
}

void sendUDP() {
    udp.beginPacket(client_address, client_port);
    for (int i = 0; i < SENDDATANUM; i++) {
        udp.write(s_ufdata[i].binary, sizeof(uf));
    }
    udp.endPacket();
}

void loop() {
    // memo
    // * だいぶいいとこまできた。ネット越しにデータは受け取れている
    // * 理由は不明だが、左回転が止まらない
    // * ↑の問題はあるが入ってきている値は正常な感じがしていて、傾けた分だけ傾く
    // * どこかで打ち消す計算式がある？M5StickCが壊れている？
    if (gyroOffsetInstalled) {
        s_ufdata[0].fval = imuData.quat[0];
        s_ufdata[1].fval = imuData.quat[1];
        s_ufdata[2].fval = imuData.quat[2];
        s_ufdata[3].fval = imuData.quat[3];

        sendUDP();
    }

    delay(10);
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
