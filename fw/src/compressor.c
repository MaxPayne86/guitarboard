/*
 * This is the main file defining the software that goes into an effects box
 * built in a wah-wah pedal case. It is controlled by one mode selection knob,
 * four parameter knobs and the pedal position.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include  "dsp/basics.h"
#include "dsp/dynamic.h"
#include "platform.h"
#include "utils.h"

uint16_t fxSelector = 0, old_fxSelector = 0;
uint16_t currentEffect = 0;

static void feedthrough(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out)
{
    *out = *in;
}

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    setLed(LED_GREEN, true);

    FloatAudioBuffer fin;
    samplesToFloat(in, &fin);
    FloatAudioBuffer fout = { .m = { 0 } };

    switch (currentEffect)
    {
        case 16:
            // Fade out whatever is still in the output buffers
            for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
                out->m[s] = out->m[s] >> 1;
            }
            break;

        case 0:
            processGain(&fin, &fout, 2.264f);
            break;

        case 1:
            processGain(&fin, &fout, 1.0f);
            break;

        case 2:
            processGain(&fin, &fout, 1.0f);
            break;

        default:
            feedthrough(&fin, &fout);
                break;
    }
    
    if (willClip(&fout)) {
        setLed(LED_RED, true);
    }
    else {
        setLed(LED_RED, false);
    }

    floatToSamples(&fout, out);

    setLed(LED_GREEN, false);
}

static void userCallback()
{
    // This function runs @ 250ms

    printf("Hello!\n\r");
    
    fxSelector = selectorwithpot(knob(0), 4);

    if(old_fxSelector != fxSelector)
    {
        //currentEffect = 16; // Fade Out
        //for (unsigned i = 0; i < 100000; i++) __asm__("nop");

        currentEffect = fxSelector;
            
        old_fxSelector = fxSelector;
    }
}


static void idleCallback()
{
    // This function runs @ max processor speed
}

int main()
{
    platformInit(NULL);

    printf("Starting Aida DSP mini\n");

    platformRegisterUserCallback(userCallback);
    platformRegisterIdleCallback(idleCallback);
    codecRegisterProcessFunction(process);

    platformMainloop();

    return 0;
}
