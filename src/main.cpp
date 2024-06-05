#include <Arduino.h>
#include <math.h>  // Include the math library for log and pow functions

const int pwm_pin = 2; // Choice of the PWM out pin: any pin can be used by referring to the GPIO matrix

#define sig_1_freq 8621             // Frequency of ~116µs
#define sig_0_freq 5000             // Frequency of 200µs
const int pin_channel = 0;          // Choice of the PWM channel = 0
unsigned int sig_resolution = 1;    // Resolution = 1 bit => 2^1 = 0, 1, 2
unsigned int res_max_value;         // Maximum value of the resolution
unsigned long sig_duty = 1;         // Duty cycle of 1/2 or 50% (1 is 50% of 1/2^1)
uint32_t sig_freq = sig_0_freq;     // Set the frequency
int percent = 50;                   // Duty cycle percentage

void setup() {
    Serial.begin(115200);
    delay(200);

    // Calculation of settings for each frequency band
    // Resolution = log2(Clock(80MHz)/f) + 1
    sig_resolution = int(log((80000000.0 / sig_freq) + 1) / log(2));
    Serial.print("sig_resolution: "); Serial.println(sig_resolution);

    res_max_value = pow(2, sig_resolution) - 1;

    // Setup PWM
    ledcAttachPin(pwm_pin, pin_channel); // Attach pin 19 to PWM channel
    bool setup_success = ledcSetup(pin_channel, sig_freq, sig_resolution); // Setup frequency and resolution for the channel

    if (setup_success) {
        Serial.println("LEDC setup successful.");
    } else {
        Serial.println("LEDC setup failed.");
    }
}

void loop() {
    // Gradually increase duty cycle from 0 to max value
    for (int duty = 0; duty <= res_max_value/32; duty++) {
        // sig_duty = (pow(2, sig_resolution) * duty) / 100;
        ledcWrite(pin_channel, duty);
        delay(4); // Delay to create a visible effect
    }

    // Gradually decrease duty cycle from max_value to 0
    for (int duty = res_max_value/32; duty >= 0; duty--) {
        // sig_duty = (pow(2, sig_resolution) * duty) / 100;
        ledcWrite(pin_channel, duty);
        delay(8); // Delay to create a visible effect
    }
}
