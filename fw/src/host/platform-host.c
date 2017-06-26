#include "platform.h"
#include "jackclient.h"

static int32_t Pulses = 0;
static float regulation_precision = 0.1; // decimal increment/decrement per encoder pulse used in processencoder
uint8_t enc_freeze = 0x00; // Free run

void platformInit(const KnobConfig* knobConfig)
{
    (void)knobConfig;
    jackClientInit();
}

void platformRegisterUserCallback(void(*cb)(void))
{
    jackClientSetIdleCallback(cb); // To be fixed to run @ 250ms
}

void platformRegisterSwitchCallback(void(*cb)(void))
{
    jackClientSetIdleCallback(cb); // To be fixed to run @ 10ms
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

// Methods for get/set encoder's pulses value
int32_t getPulses(void)
{
    return Pulses;
}

void setPulses(int32_t value)
{
    Pulses = value;
}

/**
 * Set fine or rough regulation for processencoder function
 * @param precision set decimal increment/decrement per encoder pulse
 */       
void set_regulation_precision(float precision)
{
  regulation_precision = precision;
}

/**
 * Get regulation precision setting used with processencoder function
 * @return regulation precision 
 */       
float get_regulation_precision(void)
{
  return regulation_precision;
}

/**
 * This function transform pulses from encoder in user defined range values. 
 * At startup Pulses = 0 since most common knob encoder types are relative, and
 * this function returns zero/minval. Pulses can be initialized with setPulses() function. 
 * @param minval
 * @param maxval
 * @return float - return a value between minval and maxval when user turn encoder knob
 */
float processencoder(float minval, float maxval)
{
  float tmp = 0.00;
  
  tmp = (Pulses*regulation_precision);
  
  if(tmp>maxval)
  {
    tmp = maxval;
    enc_freeze = 0x01; // Stop counting positive
  }
  else if(tmp<minval)
  {
    tmp = minval;
    enc_freeze = 0x10; // Stop counting negative
  }
  else
  {
    enc_freeze = 0x00; // Free run
  }
  
  return tmp;
}

/**
 * This function transform pulses from encoder in a selector which returns integer indexes 
 * useful for mux switch operation or menu entries
 * @param pulses - the actual pulses count see getPulses()
 * @param bits - number of bits for selector: 2=1:4, 3=1:8, etc...
 * @return uint16 - return a value between minval and maxval when user turn encoder knob
 */
uint16_t selectorwithencoder(int32_t pulses, uint8_t bits)
{
  uint16_t result = 1;
  
  if(pulses>16 && bits>0)
  {
    result = (pulses>>4)&0x0F;
    if(result > (1<<bits))
      result = (1<<bits);
  }
  
  return result;  // Because we manage encoder in 4x resolution so every step on the encoder gives 4 increment 
}