#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif

#include <Arduino.h>
#include "epd_driver.h"
#include "font_all.h"
#include "esp_adc_cal.h"
#include <FS.h>
#include <SPI.h>
#include <SD.h>
#include "logo.h"
#include "pins.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid     = "Xiaomi_1091";
const char* password = "liushichao";
#define NTP_OFFSET  28800 // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "ntp.aliyun.com"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

#if defined(T5_47_PLUS)
#include "pcf8563.h"
#include <Wire.h>
#endif

#if defined(T5_47_PLUS)
PCF8563_Class rtc;
#endif

int vref = 1100;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    char buf[128];
    /**
    * SD Card test
    * Only as a test SdCard hardware, use example reference
    * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD/examples
    */
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    // SD_MMC.setPins(SD_SCLK, SD_MOSI, SD_MISO);
    // bool rlst = SD_MMC.begin("/sdcard", true);

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    timeClient.begin();

    bool rlst = SD.begin(SD_CS, SPI);
    if (!rlst) {
        Serial.println("SD init failed");
        snprintf(buf, 128, "➸ No detected SdCard");
    } else {
        Serial.println("SD init success");
        snprintf(buf, 128,
            "➸ Detected SdCard insert:%.2f GB",
            SD.cardSize() / 1024.0 / 1024.0 / 1024.0
        );
    }
    // Correct the ADC reference voltage
    esp_adc_cal_characteristics_t adc_chars;
#if defined(T5_47)
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100,
        &adc_chars
    );
#else
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_2,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100,
        &adc_chars
    );
#endif
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("eFuse Vref: %umV\r\n", adc_chars.vref);
        vref = adc_chars.vref;
    }

#if defined(T5_47_PLUS)
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    rtc.begin();
    rtc.setDateTime(2022, 6, 30, 0, 0, 0);
#endif

    epd_init();

    Rect_t area = {
        .x = 0,
        .y = 0,
        .width = logo_width,
        .height = logo_height,
    };

    epd_poweron();
    epd_clear();
    // epd_draw_grayscale_image(area, (uint8_t *)logo_data);
    epd_draw_image(area, (uint8_t *)logo_data, BLACK_ON_WHITE);
    epd_poweroff();


    int cursor_x = 200;
    int cursor_y = 250;

    const char *string1 = "➸ 你好啊，小帅！ 16 color grayscale  😀 \n";
    const char *string2 = "➸ Use with 4.7\" EPDs 😍 \n";
    const char *string3 = "➸ High-quality font rendering ✎🙋";
    const char *string4 = "➸ ~630ms for full frame draw 🚀\n";

    epd_poweron();

    writeln((GFXfont *)&demo, buf, &cursor_x, &cursor_y, NULL);
    delay(500);
    cursor_x = 200;
    cursor_y += 50;
    writeln((GFXfont *)&demo, string1, &cursor_x, &cursor_y, NULL);
    delay(500);
    cursor_x = 200;
    cursor_y += 50;
    writeln((GFXfont *)&demo, string2, &cursor_x, &cursor_y, NULL);
    delay(500);
    cursor_x = 200;
    cursor_y += 50;
    writeln((GFXfont *)&demo, string3, &cursor_x, &cursor_y, NULL);
    delay(500);
    cursor_x = 200;
    cursor_y += 50;
    writeln((GFXfont *)&demo, string4, &cursor_x, &cursor_y, NULL);
    delay(500);

    epd_poweroff();
}

void loop()
{
    // When reading the battery voltage, POWER_EN must be turned on
    epd_poweron();
    delay(10); // Make adc measurement more accurate
    uint16_t v = analogRead(BATT_PIN);
    float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    String voltage = "➸ Voltage: " + String(battery_voltage) + "V";
#if defined(T5_47_PLUS)
    voltage = voltage + String(" (") + rtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S) + String(")");
#endif
    Serial.println(voltage);

    Rect_t area = {
        .x = 200,
        .y = 460,
#if defined(T5_47_PLUS)
        .width = 700,
#else
        .width = 320,
#endif
        .height = 50,
    };

    int cursor_x = 200;
    int cursor_y = 500;
    epd_clear_area(area);
    timeClient.update();
    String formattedTime = timeClient.getFormattedTime();
    writeln((GFXfont *)&demo, (char *)formattedTime.c_str(), &cursor_x, &cursor_y, NULL);


    /**
     * There are two ways to close
     * It will turn off the power of the ink screen,
     * but cannot turn off the blue LED light.
     */
    // epd_poweroff();

    /**
     * It will turn off the power of the entire
     * POWER_EN control and also turn off the blue LED light
     */
    epd_poweroff_all();

    delay(60000);
}
