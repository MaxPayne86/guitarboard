#pragma once
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ON 0x01
#define OFF 0x00

#define FULLRANGEVAL 4096.0f
#define MIDDLEVAL (FULLRANGEVAL/2)

/**
 * Convert a frequency to radians for making filters (normalized in respect to the sampling frequency)
 */
#define HZ2OMEGA(f) ((f) * (3.1416f / NYQUIST))

/**
 * dB - linear amplitude conversions
 */
#define DB_TO_LINEAR(x) (pow(10.0, (x) / 20.0))
#define LINEAR_TO_DB(x) (20.0 * log10(x))

/**
 * Convert a 16-bit unsigned to a float value in the provided range, linearly.
 */
#define RAMP_U16(v, start, end) ((end * ((float)(v))/UINT16_MAX) + \
        (start * ((float)(UINT16_MAX-(v)))/UINT16_MAX))

#define RAMP(v, start, end) ((end) * (v) + (start * (1.0f-(v))))

#define CLAMP(v, min, max) (v) > (max) ? (max) : ((v) < (min) ? (min) : (v))

/**
 * This function transform values from pot mounted on adc input in user defined range values
 * @param minval
 * @param maxval
 * @param potval - the actual adc value returned by analogRead(Ax)
 * @return float - return a value between minval and maxval when user turn a pot mounted on adc input
 */
static inline float processpot(float minval, float maxval, uint16_t potval)
{
  if(minval < 0 && maxval <= 0)
  {
    return (((potval*((fabs(minval)-fabs(maxval))/FULLRANGEVAL)))+minval);    
  }
  else if(minval >= 0 && maxval > 0)
  {
    return ((((potval*(maxval-minval)/FULLRANGEVAL)))+minval);
  }
  else if(minval < 0 && maxval > 0)
  {
    if(potval >= MIDDLEVAL)
        return ((potval-MIDDLEVAL)*(maxval/MIDDLEVAL));
      else
        return ((MIDDLEVAL-potval)*(minval/MIDDLEVAL));
  }
}

/**
 * This function transform values from pot mounted on adc input in a selector which returns integer indexes 
 * useful for mux switch operation or menu entries
 * @param potval - the actual adc value returned by analogRead(Ax)
 * @param bits - number of bits for selector: 2=1:4, 3=1:8, etc...
 * @return uint16 - return a value between 0 and X where X = 2^bits
 */
static inline uint16_t selectorwithpot(uint16_t potval, uint8_t bits)
{
  uint16_t result = 1;
  
  if(bits>0 && bits<=12) // 12 bits ADCs
    result = potval >> (12-bits);
  
  return result;
}

/**
 * This function returns 1/true if input value is between a reference value with threshold
 * useful for know if a knob has been turned by user, without picking noise
 * @param value - the actual value you want to compare
 * @param reference - your reference value
 * @return threshold - value of threshold 
 */
static inline uint8_t isinrange(int16_t value, int16_t reference, int16_t threshold)
{
  if(value < (reference+threshold) && value > (reference-threshold))
    return 1;
  else 
    return 0;
}

static inline void samplesToFloat(const AudioBuffer* restrict in,
        FloatAudioBuffer* restrict out)
{
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        out->m[s] = in->m[s] / 32768.0f; // Scaling to have +/- 1.0
    }
}

static inline void floatToSamples(const FloatAudioBuffer* restrict in,
        AudioBuffer* restrict out)
{
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        out->m[s] = in->m[s] * 32768.0f; // Scaling to have +/- 1.0
    }
}

/**
 * Return true if there are samples that can't be represented as a 16-bit
 * integer in the buffer.
 */
static inline bool willClip(const FloatAudioBuffer* restrict in)
{
    for (unsigned s = 0; s < 2 * CODEC_SAMPLES_PER_FRAME; s++) {
        if (in->m[s] < INT16_MIN || in->m[s] > INT16_MAX) {
            return true;
        }
    }
    return false;
}

/**
 * Pick a non-integer position from a lookup table or delay line
 */
static inline float linterpolate(const CodecIntSample* table, unsigned tablelen, float pos)
{
    const unsigned intpos = (unsigned)pos;
    const float s0 = table[intpos % tablelen];
    const float s1 = table[(intpos + 1) % tablelen];
    const float frac = pos - intpos;
    return s0 * (1.0f - frac) + s1 * frac;
}

/**
 * Pick a non-integer position from a lookup table or delay line
 */
static inline float linterpolateFloat(const float* table, unsigned tablelen, float pos)
{
    const unsigned intpos = (unsigned)pos;
    const float s0 = table[intpos % tablelen];
    const float s1 = table[(intpos + 1) % tablelen];
    const float frac = pos - intpos;
    return s0 * (1.0f - frac) + s1 * frac;
}
