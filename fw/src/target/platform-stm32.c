/*
 * Platform specific details for running on STM32F405RG.
 */

#include <libopencm3/cm3/itm.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencmsis/core_cm3.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

#include "platform.h"
#include "wm8731.h"
#include "codec.h"
#include "usb.h"

#define ADC_PINS KNOB_COUNT
#define USERTICK CODEC_SAMPLERATE/4u // 250ms
#define SWITCHTICK CODEC_SAMPLERATE/100u // 10ms

// STM32F405RG pin map
static const struct {
    uint32_t port;
    uint16_t pinno;
    uint8_t adcno;
} pins[KNOB_COUNT] = {
        { GPIOA, GPIO0, 0 },
        { GPIOA, GPIO1, 1 },
        { GPIOA, GPIO2, 2 },
        { GPIOA, GPIO3, 3 },
        { GPIOC, GPIO0, 10 },
        { GPIOC, GPIO1, 11 },
};

static const KnobConfig defaultKnobConfig[KNOB_COUNT] = {
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .analog = true },
};
 
static int32_t Pulses = 0;
static float regulation_precision = 0.1; // decimal increment/decrement per encoder pulse used in processencoder
uint8_t enc_freeze = 0x00; // Free run

static const uint32_t ADC_DMA_STREAM = DMA_STREAM0;
static const uint32_t ADC_DMA_CHANNEL = DMA_SxCR_CHSEL_0;

static volatile uint16_t adcSamples[ADC_PINS]; // 12-bit value
static volatile uint16_t adcValues[ADC_PINS]; // 16-bit value

static userFunction userCallback;
static userFunction idleCallback;
static userFunction switchCallback;

static void adcInit(const KnobConfig* knobConfig)
{
    // Set up analog input pins and build a channel list
    uint8_t channels[ADC_PINS];
    size_t channelCount = 0;
    for (size_t i = 0; i < KNOB_COUNT; i++) {
        if (knobConfig[i].analog) {
            gpio_mode_setup(pins[i].port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, pins[i].pinno);
            channels[channelCount++] = pins[i].adcno;
        }
    }

    if (channelCount == 0) {
        return;
    }

    adc_power_off(ADC1);
    adc_enable_scan_mode(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_480CYC);

    adc_power_on(ADC1);

    adc_set_regular_sequence(ADC1, channelCount, channels);

    // Configure the DMA engine to stream data from the ADC.
    dma_stream_reset(DMA1, ADC_DMA_STREAM);
    dma_set_peripheral_address(DMA2, ADC_DMA_STREAM, (intptr_t)&ADC_DR(ADC1));
    dma_set_memory_address(DMA2, ADC_DMA_STREAM, (intptr_t)adcSamples);
    dma_set_number_of_data(DMA2, ADC_DMA_STREAM, channelCount);
    dma_channel_select(DMA2, ADC_DMA_STREAM, ADC_DMA_CHANNEL);
    dma_set_transfer_mode(DMA2, ADC_DMA_STREAM, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_set_memory_size(DMA2, ADC_DMA_STREAM, DMA_SxCR_MSIZE_16BIT);
    dma_set_peripheral_size(DMA2, ADC_DMA_STREAM, DMA_SxCR_PSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA2, ADC_DMA_STREAM);
    dma_enable_circular_mode(DMA2, ADC_DMA_STREAM);
    dma_enable_stream(DMA2, ADC_DMA_STREAM);

    adc_set_dma_continue(ADC1);
    adc_set_continuous_conversion_mode(ADC1);
    adc_enable_dma(ADC1);
    adc_start_conversion_regular(ADC1);
}

static void ioInit(const KnobConfig* knobConfig)
{
    for (size_t i = 0; i < KNOB_COUNT; i++) {
        if (!knobConfig[i].analog) { // Configure the pin as input pullup only if it's not configured as analog input
            gpio_mode_setup(pins[i].port, GPIO_MODE_INPUT,
                    knobConfig[i].pullup ? GPIO_PUPD_PULLUP : GPIO_PUPD_NONE,
                            pins[i].pinno);
        }
    }
}

void platformFrameFinishedCB(void)
{
    static uint32_t sum[ADC_PINS];

    // Lowpass filter analog inputs
    for (unsigned i = 0; i < ADC_PINS; i++) {
        sum[i] = ((((64u)-1) * sum[i])+((uint32_t)adcSamples[i]*(64u)))/(64u);
        adcValues[i] = (uint16_t)(sum[i]/64u);
    }
}

void platformInit(const KnobConfig* knobConfig)
{
    if (!knobConfig) {
        knobConfig = defaultKnobConfig;
    }

    rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_DMA1);
    rcc_periph_clock_enable(RCC_DMA2);
    rcc_periph_clock_enable(RCC_ADC1);

    // Enable LED pins and turn them on
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8 | GPIO7);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
    setLed(LED_GREEN, true);
    setLed(LED_RED, true);
    setLed(LED_BLUE, true);

    adcInit(knobConfig);
    ioInit(knobConfig);
    codecInit();
    usbInit();
}

uint16_t knob(uint8_t n)
{
    return adcValues[n];
}

bool button(uint8_t n)
{
    return gpio_get(pins[n].port, pins[n].pinno);
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

void platformRegisterIdleCallback(userFunction cb)
{
    idleCallback = cb;
}

void platformRegisterSwitchCallback(userFunction cb)
{
    switchCallback = cb;
}

void platformRegisterUserCallback(userFunction cb)
{
    userCallback = cb;
}

void platformMainloop(void)
{
    setLed(LED_GREEN, false);
    setLed(LED_RED, false);
    setLed(LED_BLUE, false);

    unsigned lastprint = 0;
    unsigned lastuser = 0;
    unsigned lastswitch = 0;
    
    while (true) {
        __WFI();

        if (idleCallback) {
            idleCallback();
        }
        
        if (samplecounter >= lastswitch + SWITCHTICK) // Every 10ms
        {  
            if (switchCallback) {
                switchCallback();
            }
            lastswitch += SWITCHTICK;
        }
        
        if (samplecounter >= lastuser + USERTICK) // Every 250ms
        {  
            if (userCallback) {
                userCallback();
            }
            lastuser += USERTICK;
        }

        if (samplecounter >= lastprint + CODEC_SAMPLERATE) { // Every second
            /*printf("%u samples, peak %5d %5d. ADC %4d %4d %4d %4d %4d %4d\n\r",
                    samplecounter, peakIn, peakOut,
                    adcValues[0], adcValues[1],
                    adcValues[2], adcValues[3],
                    adcValues[4], adcValues[5]);*/

            peakIn = peakOut = INT16_MIN; // Reset peak variables
            lastprint += CODEC_SAMPLERATE;
        }

        
    }
}
