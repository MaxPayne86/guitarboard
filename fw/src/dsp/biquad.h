#pragma once
#include "codec.h"

/**
 * Biquad coefficients for
 *
 *          b0 + b1*z^-1 + b2*z^-2
 *  H(z) = ------------------------
 *          a0 + a1*z^-1 + a2*z^-2
 *
 *  Where a0 has been normalized to 1.
 */
typedef struct {
    float a1, a2; // poles
    float b0, b1, b2; // zeros
} FloatBiquadCoeffs;

/**
 * Stereo state for a biquad stage.
 */
typedef struct {
    float X[2][2];
    float Y[2][2];
} FloatBiquadState;

// 2nd order equalizer defines
#define Peaking         0
#define Parametric      1
#define LowShelf        2
#define HighShelf       3
#define Lowpass         4
#define Highpass        5
#define Bandpass        6
#define Bandstop        7
#define ButterworthLP   8
#define ButterworthHP   9
#define BesselLP       10
#define BesselHP       11

typedef struct {
  float threshold;  // -90/+6 [dB]
  float ratio;      // 1 - 100
  float attack;     // 1 - 500 [ms]
  float hold;       // 1 - attack [ms]
  float decay;      // 2000 [ms]
  float postgain;   // -30/+24 [dB]
} compressor_t;

typedef struct {
  float Q;          // Parametric, Peaking, range 0-16
  float S;          // Slope (Shelving), range 0-2
  float bandwidth;   // Bandwidth in octaves, range 0-11 
  float gain;       // Range +/-15 [dB]
  float f;         // Range 20-20000 [Hz]
  float boost;      // Range +/-15 [dB]
  unsigned char type;     // See defines section...
  unsigned char phase;    // 0 or False -> in phase (0°) 1 or True -> 180°
  unsigned char onoff;    // False -> off True -> on
} equalizer_t;

typedef struct {
  float Boost_Bass_dB;
  float Boost_Treble_dB;
  float Freq_Bass;
  float Freq_Treble;
  unsigned char phase;    // 0 or False -> in phase (0°) 1 or True -> 180°    
  unsigned char onoff;    // False -> off True -> on
} toneCtrl_t;

/**
 * Create a second-order resonant lowpass filter
 *
 * @param w0 Cutoff/resonance frequency in radians
 * @param q Resonance
 */
void bqMakeLowpass(FloatBiquadCoeffs* c, float w0, float q);

/**
 * Create a second-order bandpass filter
 *
 * @param w0 Centre frequency in radians
 * @param q Bandwidth/resonance figure
 */
void bqMakeBandpass(FloatBiquadCoeffs* c, float w0, float q);

/**
 * This function manages a general 2nd order filter block
 * @param c - the struct containing biquad coefficients 
 * @param equalizer - the struct which contains multiple settings for equalizer
 */
void EQ2ndOrd(FloatBiquadCoeffs* c, equalizer_t* equalizer);

/**
 * This function manages a baxandall low-high dual tone control
 * @param c - the struct containing biquad coefficients
 * @param toneCtrl - the struct which contains multiple settings for tone control
 */
void ToneControl(FloatBiquadCoeffs* c, toneCtrl_t *toneCtrl);

/**
 * Run a biquad filter
 */
void bqProcess(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        const FloatBiquadCoeffs* c, FloatBiquadState* state);
