/**
 * @file      main.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2022  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2022-09-16
 *
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include "esp_camera.h"
#include <secrets.h>

#define XPOWERS_CHIP_AXP2101
#define VERSION "1.0.4"
#include "XPowersLib.h"
#include "utilities.h"

void startCameraServer();

XPowersPMU PMU;
WiFiMulti wifiMulti;
String hostName = "Evil-Hacker-Network";
String ipAddress = "";
bool use_ap_mode = true; //defines if we should be using an access point for connection, this is true cause its fastest

camera_config_t create_camera_config()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    // Board pin map (LILYGO T-Camera S3)
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    // Keep 20 MHz XCLK (stable for OV2640/OV5640 with esp32-camera)
    config.xclk_freq_hz = 20000000;

    // Off-device OCR: stream JPEG at high resolution & quality
    config.pixel_format = PIXFORMAT_JPEG;

    // Prefer latest frame for alignment/handheld use; put FBs in PSRAM
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;

    // Higher quality reduces ringing/blocking around glyphs (lower = better)
    config.jpeg_quality = 0; // 0..63; 8–10 is a good OCR sweet spot
    config.fb_count = 1;      // default; will bump to 2 if PSRAM

    // Tune by memory availability
    if (psramFound())
    {
        // Continuous capture with double buffers is recommended for JPEG streams
        config.fb_count = 2; // double buffering for smoother streaming
        // If you KNOW your unit has OV5640 (5MP), you may try:
        //   config.frame_size = FRAMESIZE_QSXGA;    // 2560x1920 (OV5640 max ~2592x1944)
        // but FRAMESIZE_UXGA is a universally safe default.
    }
    else
    {
        // Without PSRAM, keep size moderate and store FBs in DRAM
        config.frame_size = FRAMESIZE_SVGA; // 800x600
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.fb_count = 1;
    }

    return config;
}

void setup()
{

    Serial.begin(115200);

    // Start while waiting for Serial monitoring
    while (!Serial)
        ;

    delay(3000);

    Serial.println();

    /*********************************
     *  step 1 : Initialize power chip,
     *  turn on camera power channel
     ***********************************/
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL))
    {
        Serial.println("Failed to initialize power.....");
        while (1)
        {
            delay(5000);
        }
    }
    // Set the working voltage of the camera, please do not modify the parameters
    PMU.setALDO1Voltage(1800); // CAM DVDD  1500~1800
    PMU.enableALDO1();
    PMU.setALDO2Voltage(2800); // CAM DVDD 2500~2800
    PMU.enableALDO2();
    PMU.setALDO4Voltage(3000); // CAM AVDD 2800~3000
    PMU.enableALDO4();

    // TS Pin detection must be disable, otherwise it cannot be charged
    PMU.disableTSPinMeasure();

    /*********************************
     * step 2 : start network
     * If using station mode, please change use_ap_mode to false,
     * and fill in your account password in wifiMulti
     ***********************************/
    if (use_ap_mode)
    {

        WiFi.mode(WIFI_AP);
        hostName += WiFi.macAddress().substring(0, 5);
        WiFi.softAP(hostName.c_str());
        ipAddress = WiFi.softAPIP().toString();
        Serial.print("Started AP mode host name :");
        Serial.println(hostName);
        Serial.print("IP address is :");
        Serial.println(WiFi.softAPIP().toString());
    }
    else
    {

        wifiMulti.addAP(WIFI_SSID1, WIFI_SSID_PASSWORD1);

        Serial.println("Connecting Wifi...");
        if (wifiMulti.run() == WL_CONNECTED)
        {
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
        }
    }

    /*********************************
     *  step 3 : Initialize camera //TODO: modify the init logic
     ***********************************/
    camera_config_t config;
    config = create_camera_config(); 
    
    Serial.println("Camera config created. Initializing camera...");
    Serial.printf("Camera config:\n");
    Serial.printf("  ledc_channel: %d\n", config.ledc_channel);
    Serial.printf("  ledc_timer: %d\n", config.ledc_timer);
    Serial.printf("  pin_d0: %d\n", config.pin_d0);
    Serial.printf("  pin_d1: %d\n", config.pin_d1);
    Serial.printf("  pin_d2: %d\n", config.pin_d2);
    Serial.printf("  pin_d3: %d\n", config.pin_d3);
    Serial.printf("  pin_d4: %d\n", config.pin_d4);
    Serial.printf("  pin_d5: %d\n", config.pin_d5);
    Serial.printf("  pin_d6: %d\n", config.pin_d6);
    Serial.printf("  pin_d7: %d\n", config.pin_d7);
    Serial.printf("  pin_xclk: %d\n", config.pin_xclk);
    Serial.printf("  pin_pclk: %d\n", config.pin_pclk);
    Serial.printf("  pin_vsync: %d\n", config.pin_vsync);
    Serial.printf("  pin_href: %d\n", config.pin_href);
    Serial.printf("  pin_sscb_sda: %d\n", config.pin_sscb_sda);
    Serial.printf("  pin_sscb_scl: %d\n", config.pin_sscb_scl);
    Serial.printf("  pin_pwdn: %d\n", config.pin_pwdn);
    Serial.printf("  pin_reset: %d\n", config.pin_reset);
    Serial.printf("  xclk_freq_hz: %d\n", config.xclk_freq_hz);
    Serial.printf("  pixel_format: %d\n", config.pixel_format);
    Serial.printf("  frame_size: %d\n", config.frame_size);
    Serial.printf("  jpeg_quality: %d\n", config.jpeg_quality);
    Serial.printf("  fb_count: %d\n", config.fb_count);
    Serial.printf("  grab_mode: %d\n", config.grab_mode);
    Serial.printf("  fb_location: %d\n", config.fb_location);


    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {S
        Serial.printf("Camera init failed with error 0x%x Please check if the camera is connected well.", err);
        while (1)
        {
            delay(5000);
        }
    }
    else
    {
        Serial.printf("[Success] Running %s\n", VERSION);
    }
    delay(1000);

    sensor_t *s = esp_camera_sensor_get();

    // Orientation
    s->set_hmirror(s, 0);
    s->set_vflip(s, 1);

    //frame size
    s->set_framesize(s, FRAMESIZE_P_FHD);


    // Basic lighting scenario settings (dynamic?)
    //s->set_brightness(s, 0); // up the brightness just a bit
    //s->set_saturation(s, 0); // lower the saturation
    //s->set_contrast(s, 2);

    // Experimental improvements
    //s->set_agc_gain(s, 12); // TODO: gain ctrl? gain ceiling?
    //s->set_sharpness(s, 3);

    /*********************************
     *  step 4 : start camera web server
     ***********************************/
    startCameraServer();
}

void loop()
{
    delay(10000);
}
