/*
 * Arduino Nano ESP32 board configuration for Blynk.Edgent
 * https://docs.arduino.cc/hardware/nano-esp32
 */

#define USE_ARDUINO_NANO_ESP32

#if defined(USE_ARDUINO_NANO_ESP32)

  // Use a non-boot GPIO for the wake button to avoid USB/serial disconnects.
  // Change to a pin that doesn't affect USB/boot (e.g., GPIO25).
  #define BOARD_BUTTON_PIN 25
  #define BOARD_BUTTON_ACTIVE_LOW true

  // Built-in LED
  #define BOARD_LED_PIN 13
  #define BOARD_LED_INVERSE false
  #define BOARD_LED_BRIGHTNESS 128

#else

  #warning "Custom board configuration is used"

  #define BOARD_BUTTON_PIN 0
  #define BOARD_BUTTON_ACTIVE_LOW true

  #define BOARD_LED_PIN 13
  #define BOARD_LED_INVERSE false
  #define BOARD_LED_BRIGHTNESS 64

#endif

#define BUTTON_HOLD_TIME_INDICATION 3000
#define BUTTON_HOLD_TIME_ACTION 10000
#define BUTTON_PRESS_TIME_ACTION 50

#define BOARD_PWM_MAX 1023

#define BOARD_LEDC_CHANNEL_1 1
#define BOARD_LEDC_CHANNEL_2 2
#define BOARD_LEDC_CHANNEL_3 3
#define BOARD_LEDC_TIMER_BITS 10
#define BOARD_LEDC_BASE_FREQ 12000

#if !defined(CONFIG_VENDOR_PREFIX)
  #define CONFIG_VENDOR_PREFIX "Garden"
#endif
#if !defined(CONFIG_AP_URL)
  #define CONFIG_AP_URL "blynk.setup"
#endif
#if !defined(CONFIG_DEFAULT_SERVER)
  #define CONFIG_DEFAULT_SERVER "blynk.cloud"
#endif
#if !defined(CONFIG_DEFAULT_PORT)
  #define CONFIG_DEFAULT_PORT 443
#endif

#define WIFI_CLOUD_MAX_RETRIES 500
#define WIFI_NET_CONNECT_TIMEOUT 50000
#define WIFI_CLOUD_CONNECT_TIMEOUT 50000
#define WIFI_AP_IP IPAddress(192, 168, 4, 1)
#define WIFI_AP_Subnet IPAddress(255, 255, 255, 0)

#define USE_PTHREAD

#define BLYNK_NO_DEFAULT_BANNER

#if defined(APP_DEBUG)
  #define DEBUG_PRINT(...) BLYNK_LOG1(__VA_ARGS__)
  #define DEBUG_PRINTF(...) BLYNK_LOG(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTF(...)
#endif
