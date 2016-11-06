#include "platform.h"
#include "jackclient.h"

void platformInit(const KnobConfig* knobConfig)
{
    (void)knobConfig;
    jackClientInit();
}

void platformRegisterUserCallback(void(*cb)(void))
{
    jackClientSetIdleCallback(cb); // To be fixed to run @ 250ms
}

void platformRegisterIdleCallback(void(*cb)(void))
{
    jackClientSetIdleCallback(cb);
}

void platformMainloop(void)
{
    jackClientRun();
}

uint16_t knob(uint8_t n)
{
    (void)n;
    return 0;
}

bool button(uint8_t n)
{
    (void)n;
    return false;
}
