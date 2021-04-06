# Ryap
M5StickC sensor information (accuracy, gyro, attitude (rotation)) is transmitted by UDP on Wifi.

<img src="https://user-images.githubusercontent.com/1772636/113172205-2be91a00-9283-11eb-8870-6bb9cd06eae0.gif" width=240 />

日本語は[こちら](https://github.com/machidyo/Ryap/blob/master/README.jp.md)

## Thanks to AxisOrange 
I am using some of [AxisOrange](https://github.com/naninunenoy/AxisOrange)'s code.
I think it is better to use this for reference of connection and code via Bluetooth.

## Tools and libraries
* [PlatformIO](https://platformio.org/)
* [mahony](http://www.x-io.co.uk/node/8#open_source_ahrs_and_imu_algorithms)
* [M5StickC](https://github.com/m5stack/M5StickC) : MIT License
* [Wifi]() : esp32 Wifi support. MIT Licnese.
* [WiFiUdp]() : Library to send/receive UDP packets. MIT Licnese.

# How to use
## Setting
1. git clone
2. set the wifi information and the destination IP (set https://github.com/machidyo/Ryap/blob/master/src/main.cpp#L13-L15）
3. install to M5StickC
4. confirm to connect to Wifi (*1)

### *1 Wifi connection confirmation supplement
* Make sure you see the IP as `--ryap--192.198.137.143`
* If you cannot connect to Wifi, only `--ryap--` will be displayed. Please review the SSID, password, etc.
* IP is a sample and depends on the environment.

## Operation check
1. git clone [RypaUnity] (https://github.com/machidyo/RyapUnity)
2. Open and run in Unity.
3. Make sure the M5StickC on the Unity screen moves as the M5StickC moves.
4. If the movement is correct but the orientation is different in the first place, make the M5StickC horizontal once, press the A button to adjust the initial posture, and then check again. Demonstrated in the middle of the video.
