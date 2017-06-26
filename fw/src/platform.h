#pragma once

enum Led {
    LED_RED,
    LED_GREEN,
    LED_BLUE
};

#define KNOB_COUNT 6

#ifdef HOST
#include "host/platform-host.h"
#else
#include "target/platform-target.h"
#endif

typedef struct {
    uint8_t analog : 1;
    uint8_t pullup : 1;
} KnobConfig;

typedef void(*userFunction)(void);

void platformRegisterUserCallback(userFunction fn);
void platformRegisterSwitchCallback(userFunction fn);
void platformRegisterIdleCallback(userFunction fn);

void platformInit(const KnobConfig* knobConfig);
void platformMainloop(void);

uint16_t knob(uint8_t n);
bool button(uint8_t n);
int32_t getPulses(void);
void setPulses(int32_t value);
void set_regulation_precision(float precision);
float get_regulation_precision(void);
float processencoder(float minval, float maxval);
uint16_t selectorwithencoder(int32_t pulses, uint8_t bits);