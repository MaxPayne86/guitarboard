#include "../dsp/basics.h"
#include <math.h>
#include "codec.h"
#include "utils.h"


void processGain(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        const float gain)
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        out->s[s][0] = in->s[s][0]*gain;
        out->s[s][1] = in->s[s][1]*gain;
    }
}
