#include <Arduino.h>
#include <math.h>  // Include the math library for log and pow functions
#if defined ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#define WIFI_MODE_STA WIFI_STA 
#else
#error "Unsupported platform"
#endif //ESP32
#include <QuickEspNow.h>

static const String msg = "Hello ESP-NOW!";

#define USE_BROADCAST 1 // Set this to 1 to use broadcast communication

#if USE_BROADCAST != 1
// set the MAC address of the receiver for unicast
static uint8_t receiver[] = { 0x12, 0x34, 0x56, 0x78, 0x90, 0x12 };
#define DEST_ADDR receiver
#else //USE_BROADCAST != 1
#define DEST_ADDR ESPNOW_BROADCAST_ADDRESS 
#endif //USE_BROADCAST != 1

// Send message every 2 seconds
const unsigned int SEND_MSG_MSEC = 2000;

const int pwm_pin = 2; // Choice of the PWM out pin: any pin can be used by referring to the GPIO matrix

#define sig_1_freq 8621             // Frequency of ~116µs
#define sig_0_freq 5000             // Frequency of 200µs
const int pin_channel = 0;          // Choice of the PWM channel = 0
unsigned int sig_resolution = 1;    // Resolution = 1 bit => 2^1 = 0, 1, 2
unsigned int res_max_value;         // Maximum value of the resolution = 2 ^ resolution - 1
unsigned long sig_duty = 1;         // Duty cycle of 1/2 or 50% (1 is 50% of 1/2^1)
uint32_t sig_freq = sig_0_freq;     // Set the frequency
uint8_t percent = 50;               // Current duty cycle percentage
uint8_t prev_percent = 50;          // previous duty cycle percentage
uint8_t req_percent = 3;            // Requested duty cycle percentage

void dataReceived (uint8_t* address, uint8_t* data, uint8_t len, signed int rssi, bool broadcast) {
    Serial.print ("Bytes received: ");
    Serial.println (len);
    Serial.printf ("%.*s\n", len, data);
    Serial.printf ("RSSI: %d dBm\n", rssi);
    Serial.printf ("From: " MACSTR "\n", MAC2STR (address));
    Serial.printf ("%s\n", broadcast ? "Broadcast" : "Unicast");
    req_percent = data[0];
    Serial.print("req_percent: "); Serial.println(req_percent);
}

void setup() {
    Serial.begin(115200);
    WiFi.mode (WIFI_MODE_STA);
#if defined ESP32
    WiFi.disconnect (false, true);
#elif defined ESP8266
    WiFi.disconnect (false);
#endif //ESP32
    Serial.printf ("Connected to %s in channel %d\n", WiFi.SSID ().c_str (), WiFi.channel ());
    Serial.printf ("IP address: %s\n", WiFi.localIP ().toString ().c_str ());
    Serial.printf ("MAC address: %s\n", WiFi.macAddress ().c_str ());
    quickEspNow.onDataRcvd (dataReceived);
#ifdef ESP32
    quickEspNow.setWiFiBandwidth (WIFI_IF_STA, WIFI_BW_HT20); // Only needed for ESP32 in case you need coexistence with ESP8266 in the same network
#endif //ESP32
    quickEspNow.begin (1); // If you use no connected WiFi channel needs to be specified

    delay(200);

    // Calculation of settings for each frequency band
    // Resolution = log2(Clock(80MHz)/f) + 1
    sig_resolution = int(log((80000000.0 / sig_freq) + 1) / log(2));
    Serial.print("sig_resolution: "); Serial.println(sig_resolution);

    res_max_value = pow(2, sig_resolution) - 1;

    // Setup PWM
    ledcAttachPin(pwm_pin, pin_channel); // Attach pin to PWM channel
    bool setup_success = ledcSetup(pin_channel, sig_freq, sig_resolution); // Setup frequency and resolution for the channel

    if (setup_success) {
        Serial.println("LEDC setup successful.");
    } else {
        Serial.println("LEDC setup failed.");
    }

    sig_duty = res_max_value * percent / 100;
    ledcWrite(pin_channel, sig_duty);
}

void loop() {
    while (percent != req_percent) {
        if (percent < req_percent) {
            sig_duty++;
        } else {
            sig_duty--;
        }
        percent = sig_duty * 100 / res_max_value;
        // sig_duty = res_max_value * percent / 100;
        ledcWrite(pin_channel, sig_duty);
        delay(1);
        if (percent != prev_percent) {
            Serial.print("percent: "); Serial.println(percent);
        }
        prev_percent = percent;
    }
}
