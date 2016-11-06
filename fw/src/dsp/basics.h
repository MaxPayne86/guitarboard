#pragma once
#include "codec.h"
#include "basics.h"


void processGain(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        const float gain);
