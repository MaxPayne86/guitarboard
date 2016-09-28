#include "../dsp/1storder.h"

#include "../dsp/biquad.h"

#include <math.h>

#include "codec.h"

#include "utils.h"

/**
 * This function manages a general 1st order filter block
 * @param c - the struct containing biquad coefficients
 * @param equalizer - the struct which contains multiple settings for equalizer 
 */
void EQ1stOrd(Float1storderCoeffs* c, equalizer_t* equalizer){

  float w0,gainLinear;
  float b0=0,b1=0,a1=0;

  w0=HZ2OMEGA(equalizer->f);		          //2*pi*f/CODEC_SAMPLERATE
  gainLinear = pow(10,(equalizer->gain/20));      //10^(gain/20)

  switch(equalizer->type)
  {
    case Lowpass:
      a1 = -pow(2.7,-w0);
      b0 = gainLinear * (1.0 + a1);
      b1 = 0;
      break;   
    case Highpass:
      a1 = -pow(2.7,-w0);
      b0 = gainLinear * a1;
      b1 = -a1 * gainLinear;
      break;
  }

  if(equalizer->onoff == true)
  {
    if(equalizer->phase == false) // 0°
    {
      c->b0 = b0;
      c->b1 = b1;
      c->a1 = a1;			
    }
    else // 180°
    {
      c->b0 = -1*b0;
      c->b1 = -1*b1;
      c->a1 = a1; // This coefficient does not change sign
    }
  }
  else
  {
    c->b0 = 1.00;
    c->b1 = 0.00;
    c->a1 = 0.00;
  }

  /*// !!!Debug!!!
  printf("\n%.3f\n", c->b0);
  printf("%.3f\n", c->b1);
  printf("%.3f\n", c->a1);*/
}

void filter1storderProcess(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        const Float1storderCoeffs* c, Float1storderState* state)
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        float y[2];
        y[0] = c->b0*in->s[s][0] +
                c->b1*state->X[0][0] - c->a1*state->Y[0][0];
        y[1] = c->b0*in->s[s][1] +
                c->b1*state->X[0][1] - c->a1*state->Y[0][1];
        out->s[s][0] = y[0];
        out->s[s][1] = y[1];
        state->X[0][0] = in->s[s][0];
        state->Y[0][0] = y[0];
        state->X[0][1] = in->s[s][1];
        state->Y[0][1] = y[1];
    }
}
