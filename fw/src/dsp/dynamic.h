#pragma once
#include "codec.h"

/* Struct with standard compressor parameters */
typedef struct {
  float threshold;  // -90/+6 [dB]
  float ratio;      // 1 - 100
  float attack;     // 1 - 500 [ms]
  float hold;       // 1 - attack [ms]
  float decay;      // 2000 [ms]
  float postgain;   // -30/+24 [dB]
} compressor_t;

void dynamicProcess(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        compressor_t* c);
