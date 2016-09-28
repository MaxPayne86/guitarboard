#include "../dsp/biquad.h"

#include <math.h>

#include "codec.h"

#include "utils.h"

/*
 * Some ideas from:
 * Cookbook formulae for audio EQ biquad filter coefficients
 * by Robert Bristow-Johnson  <rbj@audioimagination.com>
 * http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
 */

void bqMakeLowpass(FloatBiquadCoeffs* c, float w0, float q)
{
    /* The cookbook says:
            b0 =  (1 - cos(w0))/2
            b1 =   1 - cos(w0)
            b2 =  (1 - cos(w0))/2
            a0 =   1 + alpha
            a1 =  -2*cos(w0)
            a2 =   1 - alpha
     */

    const float alpha = sinf(w0)/(2.0f*q);
    const float a0 = 1.0f + alpha;
    const float a0inv = 1.0 / a0;
    const float cosw0 = cosf(w0);
    
    c->a1 = -2.0f * cosw0 * a0inv;
    c->a2 = (1.0f - alpha) * a0inv;
    c->b1 = (1.0f - cosw0) * a0inv;
    c->b0 = c->b1 * 0.5f;
    c->b2 = c->b0;
}

void bqMakeBandpass(FloatBiquadCoeffs* c, float w0, float q)
{
    /* The cookbook says:
            b0 =   sin(w0)/2  =   Q*alpha
            b1 =   0
            b2 =  -sin(w0)/2  =  -Q*alpha
            a0 =   1 + alpha
            a1 =  -2*cos(w0)
            a2 =   1 - alpha
     */

    const float alpha = sinf(w0)/(2.0f*q);
    const float a0 = 1.0f + alpha;
    const float a0inv = 1.0 / a0;
    const float cosw0 = cosf(w0);
    
    c->a1 = -2.0f * cosw0 * a0inv;
    c->a2 = (1.0f - alpha) * a0inv;
    c->b0 = q * alpha * a0inv;
    c->b1 = 0;
    c->b2 = -c->b0;
}

/**
 * This function manages a general 2nd order filter block
 * @param c - the struct containing biquad coefficients
 * @param equalizer - the struct which contains multiple settings for equalizer
 */
void EQ2ndOrd(FloatBiquadCoeffs* c, equalizer_t* equalizer){

  float A,w0,alpha,gainLinear;
  float b0=0,b1=0,b2=0,a0=0,a1=0,a2=0;
    
  A=pow(10,(equalizer->boost/40));                //10^(boost/40)
  w0=HZ2OMEGA(equalizer->f);		          //2*pi*f/CODEC_SAMPLERATE
  gainLinear = pow(10,(equalizer->gain/20));      //10^(gain/20)

  switch(equalizer->type)
  {
    case Parametric:
    case Peaking: // Peaking filter is a Parametric filter with fixed Q???
      alpha = sin(w0)/(2*equalizer->Q); 
      a0 =  1 + alpha/A;
      a1 = -2 * cos(w0);
      a2 =  1 - alpha/A;
      b0 = (1 + alpha*A) * gainLinear;
      b1 = -(2 * cos(w0)) * gainLinear;
      b2 = (1 - alpha*A) * gainLinear;
      break;
    case LowShelf:
      alpha=sin(w0)/2*sqrt((A+1/A)*(1/equalizer->S-1)+2);
      a0 =  (A+1)+(A-1)*cos(w0)+2*sqrt(A)*alpha;
      a1 = -2*((A-1)+(A+1)*cos(w0));
      a2 =  (A+1)+(A-1)*cos(w0)-2*sqrt(A)*alpha;
      b0 = A*((A+1)-(A-1)*cos(w0)+2*sqrt(A)*alpha)*gainLinear;
      b1 = 2*A*((A-1)-(A+1)*cos(w0)) * gainLinear;
      b2 = A*((A+1)-(A-1)*cos(w0)-2*sqrt(A)*alpha)*gainLinear;
      break;
    case HighShelf:
      alpha = sin(w0)/2 * sqrt( (A + 1/A)*(1/equalizer->S - 1) + 2 ); 
      a0 =       (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha;
      a1 =   2*( (A-1) - (A+1)*cos(w0) );
      a2 =       (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha;
      b0 =   A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha ) * gainLinear;
      b1 = -2*A*( (A-1) + (A+1)*cos(w0) ) * gainLinear;
      b2 =   A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha ) * gainLinear;
      break;
    case Lowpass:
      alpha = sin(w0)/(2*equalizer->Q);
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =  (1 - cos(w0)) * (gainLinear/2);
      b1 =   1 - cos(w0)  * gainLinear;
      b2 =  (1 - cos(w0)) * (gainLinear/2);
      break;
    case Highpass:
      alpha = sin(w0)/(2*equalizer->Q);
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =  (1 + cos(w0)) * (gainLinear/2);
      b1 = -(1 + cos(w0)) * gainLinear;
      b2 =  (1 + cos(w0)) * (gainLinear/2);
      break;
    case Bandpass:
      alpha = sin(w0) * sinh(log(2)/(2 * equalizer->bandwidth * w0/sin(w0)));
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =   alpha * gainLinear;
      b1 =   0;
      b2 =  -alpha * gainLinear;
      break;
    case Bandstop:
      alpha = sin(w0) * sinh( log(2)/(2 * equalizer->bandwidth * w0/sin(w0)));
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =   1 * gainLinear;
      b1 =  -2*cos(w0) * gainLinear;  
      b2 =   1 * gainLinear;
      break;
    case ButterworthLP:
      alpha = sin(w0) / 2.0 * 1/sqrt(2);
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =  (1 - cos(w0)) * gainLinear / 2;
      b1 =   1 - cos(w0)  * gainLinear;
      b2 =  (1 - cos(w0)) * gainLinear / 2;
      break;
    case ButterworthHP:
      alpha = sin(w0) / 2.0 * 1/sqrt(2);
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 = (1 + cos(w0)) * gainLinear / 2;
      b1 = -(1 + cos(w0)) * gainLinear;
      b2 = (1 + cos(w0)) * gainLinear / 2;
      break;
    case BesselLP:
      alpha = sin(w0) / 2.0 * 1/sqrt(3) ;
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 =  (1 - cos(w0)) * gainLinear / 2;
      b1 =   1 - cos(w0)  * gainLinear;
      b2 =  (1 - cos(w0)) * gainLinear / 2;
      break;
    case BesselHP:
      alpha = sin(w0) / 2.0 * 1/sqrt(3) ;
      a0 =   1 + alpha;
      a1 =  -2*cos(w0);
      a2 =   1 - alpha;
      b0 = (1 + cos(w0)) * gainLinear / 2;
      b1 = -(1 + cos(w0)) * gainLinear;
      b2 = (1 + cos(w0)) * gainLinear / 2;
      break;
  }
  
  // In current implementation we need to normalize all the coefficients respect to a0  
  if(a0 != 0.00 && equalizer->onoff == true)
  {
    if(equalizer->phase == false) // 0°
    {
      c->b0=b0/a0;
      c->b1=b1/a0;
      c->b2=b2/a0;
      c->a1=a1/a0;
      c->a2=a2/a0;
    }
    else // 180°
    {
      c->b0=-1*b0/a0;
      c->b1=-1*b1/a0;
      c->b2=-1*b2/a0;
      c->a1=a1/a0; // This coefficient does not change sign!
      c->a2=a2/a0; // This coefficient does not change sign!
    }
  }
  else // off or disable position
  {
    c->b0=1.00;
    c->b1=0;
    c->b2=0;
    c->a1=0;
    c->a2=0;
  }
  
  /*// !!!Debug!!!
  printf("\n%.3f\n", c->b0);
  printf("%.3f\n", c->b1);
  printf("%.3f\n", c->b2);
  printf("%.3f\n", c->a1);
  printf("%.3f\n", c->a2);*/
}

/**
 * This function manages a baxandall low-high dual tone control
 * @param c - the struct containing biquad coefficients
 * @param toneCtrl - the struct which contains multiple settings for tone control
 */
void ToneControl(FloatBiquadCoeffs* c, toneCtrl_t *toneCtrl){
 
  float tb=0,bb=0,wT=0,wB=0,Knum_T=0,Kden_T=0,Knum_B=0,Kden_B=0,alpha0=0,beta1=0,alpha1=0,beta2=0,alpha2=0,beta3=0,alpha3=0,beta4=0;
  float b0=0,b1=0,b2=0,a0=0,a1=0,a2=0;
  
  tb = pow(10, toneCtrl->Boost_Treble_dB/20.0);
  bb = pow(10, toneCtrl->Boost_Bass_dB/20.0);

  wT = tan(M_PI * toneCtrl->Freq_Treble / CODEC_SAMPLERATE);
  wB = tan(M_PI * toneCtrl->Freq_Bass / CODEC_SAMPLERATE);

  Knum_T = 2 / (1 + (1.0 / tb));
  Kden_T = 2 / (1 + tb);
  Knum_B = 2.0 / (1.0 + (1.0 / bb));
  Kden_B = 2.0 / (1.0 + bb);

  alpha0 = wT + Kden_T;
  beta1 = wT + Knum_T;
  alpha1 = wT - Kden_T; 
  beta2 = wT - Knum_T;

  alpha2 = (wB*Kden_B) + 1;
  beta3 = (wB*Knum_B) - 1;
  alpha3 = (wB*Kden_B) - 1;
  beta4 = (wB*Knum_B) + 1;
  
  a0 = alpha0 * alpha2;
  a1 = (alpha0 * alpha3) + (alpha1 * alpha2);
  a2 = alpha1 * alpha3; 
  b0 = beta1 * beta3;
  b1 = (beta1 * beta4) + (beta2 * beta3);
  b2 = beta2 * beta4; 
  
  // In current implementation we need to normalize all the coefficients respect to a0  
  if(a0 != 0.00 && toneCtrl->onoff == true)
  {
    if(toneCtrl->phase == false) // 0°
    {
      c->b0=b0/a0;
      c->b1=b1/a0;
      c->b2=b2/a0;
      c->a1=a1/a0;
      c->a2=a2/a0;
    }
    else // 180°
    {
      c->b0=-1*b0/a0;
      c->b1=-1*b1/a0;
      c->b2=-1*b2/a0;
      c->a1=a1/a0; // This coefficient does not change sign!
      c->a2=a2/a0; // This coefficient does not change sign!
    }
  }
  else // off or disable position
  {
    c->b0=1.00;
    c->b1=0;
    c->b2=0;
    c->a1=0;
    c->a2=0;
  }
  
  /*// !!!Debug!!!
  printf("\n%.3f\n", c->b0);
  printf("%.3f\n", c->b1);
  printf("%.3f\n", c->b2);
  printf("%.3f\n", c->a1);
  printf("%.3f\n", c->a2);*/
}

void bqProcess(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        const FloatBiquadCoeffs* c, FloatBiquadState* state)
{
    for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {
        float y[2];
        y[0] = c->b0*in->s[s][0] +
                c->b1*state->X[0][0] + c->b2*state->X[1][0] -
                c->a1*state->Y[0][0] - c->a2*state->Y[1][0];
        y[1] = c->b0*in->s[s][1] +
                c->b1*state->X[0][1] + c->b2*state->X[1][1] -
                c->a1*state->Y[0][1] - c->a2*state->Y[1][1];
        out->s[s][0] = y[0];
        out->s[s][1] = y[1];
        state->X[1][0] = state->X[0][0];
        state->X[0][0] = in->s[s][0];
        state->Y[1][0] = state->Y[0][0];
        state->Y[0][0] = y[0];
        state->X[1][1] = state->X[0][1];
        state->X[0][1] = in->s[s][1];
        state->Y[1][1] = state->Y[0][1];
        state->Y[0][1] = y[1];
    }
}
