#include "../dsp/biquad.h"

#include <math.h>

#include "codec.h"

#include "utils.h"

#include "dynamic.h"


void dynamicProcess(const FloatAudioBuffer* restrict in, FloatAudioBuffer* restrict out,
        compressor_t* c)
{
	
	float attTime = c->attack*1e-3; // From ms to sec
	float relTime = c->decay*1e-3;

	float sampleRate = CODEC_SAMPLERATE;
	
	float noise_thr_dB = -40.0;
	float noise_thr_l = DB_TO_LINEAR(noise_thr_dB);

	float ga = exp(-1.0f/(sampleRate*attTime));
	float gr = exp(-1.0f/(sampleRate*relTime));	
	
	float envIn = 0.0f;
	float envIn_r = 0.0f;
	float envIn_l = 0.0f;
	float out_r = 0.0f;
	float out_l = 0.0f;
	float envRms = 0.0f;
	static float envOut = 0.0f;
	float env = 0.0f;

	float thr_l = DB_TO_LINEAR(c->threshold);
	float ratio = c->ratio;
	float cmpcoeff = 0.0f;
	float cmpgain = 1.0f;
	float postgain = DB_TO_LINEAR(c->postgain);
	
	if(ratio > 1)
		cmpcoeff = 1.0 - (1.0/ratio);
	else
		cmpcoeff = 0.0;
		
  for (unsigned s = 0; s < CODEC_SAMPLES_PER_FRAME; s++) {

		// Peak envelope detector
		/*envIn_r = fabsf(in->s[s][0]);
		envIn_l = fabsf(in->s[s][1]);

		envIn = fmaxf(envIn_r, envIn_l);

    if( envOut < envIn )
        envOut = envIn + ga * (envOut - envIn);
    else
        envOut = envIn + gr * (envOut - envIn);*/

		// Rms envelope detector
		envIn_r = fabsf(in->s[s][0]);
		envIn_l = fabsf(in->s[s][1]);

		envIn = fmaxf(envIn_r, envIn_l);
		envIn = envIn * envIn; 
		
		if( envOut < envIn )
        envOut = envIn + ga * (envOut - envIn);
    else
        envOut = envIn + gr * (envOut - envIn);
		 
		envRms = sqrtf(envOut);
		
		env = envRms;
		
		if(env < noise_thr_l)
			env = noise_thr_l;

		// Compressor calculations
		cmpgain = pow((thr_l/env), cmpcoeff);
		if(cmpgain > 1.0f)
			cmpgain = 1.0f;
		
		out_r = in->s[s][0] * cmpgain;
		out_l = in->s[s][1] * cmpgain;

		// Makeup gain
		out->s[s][0] = out_r * postgain;
		out->s[s][1] = out_l * postgain;
  }
}
