# esp32-homekit

This project is impemented Apple Homekit Accessory Protocol(HAP) to ESP32. You can make your own Homekit accessory with ESP32 with this project.

This project is modified from [younghyunjo](https://github.com/younghyunjo/esp32-homekit.git), which is not working on my esp32 devkitc/wrover-kit. So I modify it to make it work on my board.


# Demo

**light/switch**：

![](https://rillhudev.coding.net/p/blogres/d/blogres/git/raw/master/esp32-homekit.png)



temperature/humidity:





# Resource

- [Apple Homekit Accessory Protocol](https://developer.apple.com/support/homekit-accessory-protocol/)
- [Mongoose](https://github.com/cesanta/mongoose)

# Prerequisite
The `esp32-homekit` is using esp-idf libraries and build. Please download ESP-IDF and the corresponding compiling tools.

- ESP-IDF

  - Based on [ESP-IDF Release v3.3 (LTS) ](https://github.com/espressif/esp-idf/releases/tag/v3.3),Documentation for release v3.3 is available at https://docs.espressif.com/projects/esp-idf/en/v3.3/ 
  - This archive can also be downloaded from Espressif's download server:
    https://dl.espressif.com/dl/esp-idf/releases/esp-idf-v3.3.zip

- Compile tool

  https://dl.espressif.com/dl/esp32_win32_msys2_environment_and_toolchain-20181001.zip

# Configuration
## WiFi
esp32-homekit uses WiFi as tranmission layer.
To connection WiFi, you MUST config WiFi ssid and password.

1. Open examples/switch/main/main.c
2. Change EXAMPLE_ESP_WIFI_SSID, and EXAMPLE_ESP_WIFI_PASS

```
#define EXAMPLE_ESP_WIFI_SSID "123"
#define EXAMPLE_ESP_WIFI_SSID "12345678"  
```

Or you you can change it via `make menuconfig` and change the `example config`.

# Build

```
$ cd examples/switch
$ make
$ make flash
```

# Setup Code
While pairing accessory and iOS devices, You must enter Setup Code at HOME App.
The default setupt code is 
## **`053-58-917`**

Then you can control your device by `Home` app or `Siri`.



# makefile的一些说明

- 在$(patsubst %.c,%.o,$(dir) )中，patsubst把$(dir)中的变量符合后缀是.c的全部替换成.o，任何输出。

  > [makefile中的patsubst](https://blog.csdn.net/srw11/article/details/7516712)

