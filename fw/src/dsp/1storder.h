#pragma once
#include "codec.h"
#include "biquad.h"

/**
 * Filter coefficients for
 *
 *          b0 + b1*z^-1 
 *  H(z) = --------------
 *          a0 + a1*z^-1 
 *
 *  Where a0 has been normalized to 1.
 */
typedef struct {
    float a1; // poles
    float b0, b1; // zeros
} Float1storderCoeffs;

/**
 * Stereo state for a biquad stage.
 */
typedef struct {
    float X[1][1];
    float Y[1][1];
} Float1storderState;

/**
 * This function manages a general 1st order filter block
 * @param c - the struct containing biquad coefficients
 * @param equalizer - the struct which contains multiple settings for equalizer 
 */
void EQ1stOrd(Float1storderCoeffs* c, equalizer_t* equalizer);

void filter1storderProcess(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        const Float1storderCoeffs* c, Float1storderState* state);
