/*
 * This is the main file defining the software that goes into an effects box
 * built in an hammond box 1590B pedal case. It is controlled by one preset knob
 * and the footswitch for fx on/off.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec.h"
#include  "dsp/basics.h"
#include "dsp/dynamic.h"
#include "dsp/biquad.h"
#include "platform.h"
#include "utils.h"

// DEFINES USER INTERFACE
#define PREGAIN_MAX 12.0f // dB
#define PREGAIN_MIN 0.0f // dB
#define PRECISION_1 0.1f

#define THR_MAX 6.0f // dB
#define THR_MIN -96.0f // dB
#define PRECISION_2 0.1f

#define RATIO_MAX 16.0f
#define RATIO_MIN 1.0f
#define PRECISION_3 0.25f

#define ATTACK_MAX 500.0f // ms
#define ATTACK_MIN 1.0f // ms
#define PRECISION_4 0.5f

#define HOLD_MAX ATTACK_MAX
#define HOLD_MIN 1.0f // ms
#define PRECISION_5 0.5f

#define DECAY_MAX 2000.0f // ms
#define DECAY_MIN 1.0f // ms
#define PRECISION_6 1.0f

#define BRIGHT_MAX 18.0f // dB
#define BRIGHT_MIN -18.0f  // dB
#define PRECISION_7 0.1f

#define MASTER_VOLUME_MAX 0.00
#define MASTER_VOLUME_MIN -80.00
#define PRECISION_9 0.1f

#define POSTGAIN_MAX 24.0f // dB
#define POSTGAIN_MIN -30.0f // dB
#define PRECISION_10 0.1f

#define F_MIN 500.0f // Hz
#define F_MAX 12000.0f // Hz
#define PRECISION_11 50.0f

#define N_PRESET  4  // Not used
typedef enum {
  maxfunkyelectricguitar, 
  andreaelectricbass, 
  andreaerocklectricguitar, 
  andreaacousticguitar
}preset_t;

// FUNCTION PROTOTYPES
void clearAndHome(void);
void setBrightPrePost(uint8_t);
void setNotchEq(float);
void switchPreset(void);
void changeValue(void);

void print_menu_putty(void);

// GLOBAL VARIABLES
preset_t preset = maxfunkyelectricguitar, old_preset = maxfunkyelectricguitar;
uint8_t switchstatus = false;
uint8_t bypass = true;
uint8_t func_counter = 0, old_func_counter = 0;
int32_t tmpencoderpulses = 0;
float pregain = 1.0f;
float postgain = 1.0f;
float envelope = 0.0f;
compressor_t cmp1;
equalizer_t pre, post, notch;
FloatBiquadCoeffs precoeff, postcoeff, notchcoeff;
FloatBiquadState prestate, poststate, notchstate;
const KnobConfig myKnobConfig[KNOB_COUNT] = {
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .analog = true },
        { .pullup = true }, // Input footswitch pullup
};
// Values in param pulses are startup values for
// DSP Blocks
int32_t param1_pulses = 0; // Pre Gain
int32_t param2_pulses = 0; // Threshold
int32_t param3_pulses = 0; // Ratio
int32_t param4_pulses = 0; // Attack
int32_t param5_pulses = 0; // Hold
int32_t param6_pulses = 0; // Decay
int32_t param7_pulses = 0; // Bright
int32_t param8_pulses = 0; // PrePost Bright
int32_t param9_pulses = 0; // Master Volume
int32_t param10_pulses = 0; // Post Gain
int32_t param11_pulses = 0; // Parametric Eq Frequency

uint8_t restore = 1;  // If 1 at startup value triggered by func_counter is written to the DSP

float param1_value = 0.00; 
float param2_value = 0.00; 
float param3_value = 0.00; 
float param4_value = 0.00; 
float param5_value = 0.00; 
float param6_value = 0.00;
float param7_value = 0.00;
uint8_t param8_value = 0;
float param9_value = 0.00;
float param10_value = 0.00;
float param11_value = 0.00;

uint8_t inByte = 0x00;

static void fx1(const FloatAudioBuffer* restrict in,
        FloatAudioBuffer* restrict out)
{
    FloatAudioBuffer 
    tmp1 = { .m = { 0 } },
    tmp2 = { .m = { 0 } }, 
    tmp3 = { .m = { 0 } }, 
    tmp4 = { .m = { 0 } },
    tmp5 = { .m = { 0 } };
    
    processGain(in, &tmp1, pregain);
    bqProcess(&tmp1, &tmp2, &precoeff, &prestate);
    dynamicProcess(&tmp2, &tmp3, &cmp1, &envelope);
    bqProcess(&tmp3, &tmp4, &postcoeff, &poststate);
    bqProcess(&tmp4, &tmp5, &notchcoeff, &notchstate);
    processGain(&tmp5, out, postgain);
}

static void process(const AudioBuffer* restrict in, AudioBuffer* restrict out)
{
    FloatAudioBuffer fin;
    FloatAudioBuffer fout = { .m = { 0 } };

    samplesToFloat(in, &fin);
    
    switch (bypass)
    {
        case true: // Bypass
            processGain(&fin, &fout, 1.0f);
            break;

        case false: // Fx
            fx1(&fin, &fout);
            break;
    }
    
    if (willClip(&fout)) {
        setLed(LED_RED, true);
    }
    else {
        setLed(LED_RED, false);
    }

    floatToSamples(&fout, out);
}

void clearAndHome(void)
{
    putchar(0x1b); // ESC
    printf("[2J"); // clear screen
    putchar(0x1b); // ESC
    printf("[H"); // cursor to home
}

static void println(const char* string)
{
    printf("%s", string);
    putchar('\n');
    putchar('\r');
}

void print_menu_putty(void)
{
  clearAndHome();    // !!!Warning use with real terminal emulation program
  println("********************************");
  println("*    User control interface    *");
  switch(preset)
  {
    case maxfunkyelectricguitar:
  println("*    FUNKY GUIT                *");
      break;
    case andreaelectricbass:
  println("*    ELEC GUIT                 *");
      break;
    case andreaerocklectricguitar:
  println("*    ROCK GUIT                 *");
      break;
    case andreaacousticguitar:
  println("*    ACUST GUIT                *");
      break;  
  }
  println("********************************");
  putchar('\n');
  printf("Encoder pulses: %d", (int)getPulses());
  println("");
  
  // Print menu
  printf("Effect status: ");
  if(bypass)
    println("bypass");
  else
    println("on");
  putchar('\n');  
  if(func_counter==0)
    printf("    ");
  printf("PreGain: %.1f", param1_value);
  println(" dB");
  if(func_counter==1)
    printf("    ");
  printf("Thr: %.1f", param2_value);
  println(" dB");
  if(func_counter==2)
    printf("    ");
  printf("Ratio: %.2f", param3_value);
  println("");
  if(func_counter==3)
    printf("    ");
  printf("Attack: %.1f", param4_value);
  println(" ms");
  if(func_counter==4)
    printf("    ");
  printf("Hold: %.1f", param5_value);
  println(" ms");
  if(func_counter==5)
    printf("    ");
  printf("Decay: %.1f", param6_value);
  println(" ms");
  if(func_counter==6)
    printf("    ");
  printf("Bright: %.1f", param7_value);
  println(" dB");
  if(func_counter==7)
    printf("    ");
  printf("Pos: ");
  if(param8_value==1)
    println(" Pre");
  else if(param8_value==2)
    println(" Post");
  if(func_counter==8)
    printf("    ");
  printf("Vol: %.1f", param9_value);
  println(" dB");
  if(func_counter==9)
    printf("    ");
  printf("Makeup: %.1f", param10_value);
  println(" dB");
  if(func_counter==10)
    printf("    ");
  printf("Notch: %.1f", param11_value);
  println(" Hz");
  
  putchar('\n');
  printf("Active item: %d", func_counter);
  println("");
  
  fflush(stdout);
}

static void serialParser(void)
{
  inByte = getchar();
  if(inByte != '\n')
  {
    switch(inByte)
    {
      case 'q':
        func_counter = 0; 
        break;
      case 'w':
        func_counter = 1; 
        break;
      case 'e':
        func_counter = 2; 
        break;
      case 'r':
        func_counter = 3; 
        break;
      case 't':
        func_counter = 4; 
        break;
      case 'y':
        func_counter = 5; 
        break;
      case 'u':
        func_counter = 6; 
        break;
      case 'i':
        func_counter = 7; 
        break;
      case 'o':
        func_counter = 8; 
        break;
      case 'p': 
        func_counter = 9; 
        break;
      case 'a':
        func_counter = 10; 
        break;
      case '+':
        tmpencoderpulses = getPulses();
        tmpencoderpulses++;
        setPulses(tmpencoderpulses);
        break;
      case '-':
        tmpencoderpulses = getPulses();
        tmpencoderpulses--;
        setPulses(tmpencoderpulses);
        break;
      default:
        // Do nothing
        break;
    }
  }
}

static void userCallback(void)
{
    // This function runs @ 250ms
    
    bypass = !switchstatus; // switch = true, bypass = off and viceversa
    setLed(LED_GREEN, switchstatus); // Led shows pedal inserted or not
   
    preset = (uint8_t)selectorwithpot(knob(0), 4);

    if(preset != old_preset)
    {
        // Function to switch preset
        switchPreset(); 
        
        old_preset = preset;
    }
    
    changeValue();
    
    print_menu_putty();
}

static void switchCallback(void)
{
    // This function runs @ 10ms
    static uint8_t switchold = true;
    static uint8_t switchcur = 0;
    static uint8_t switchcounter = 0;
    
    switchcur = button(5);
        
    if(switchcur == false)
    {
        if(switchold == false)
        {    
            switchcounter++;
            if(switchcounter == 10)
            {
                switchstatus = true;
                switchcounter = 0;
            }
        }
        else
        {
            switchcounter = 0;
            switchstatus = false;
        }
        switchold = switchcur;
    }
    else
    {
        switchstatus = false;
    }
    
}

static void idleCallback(void)
{
    // This function runs @ max processor speed
    __asm__("nop");
    serialParser();
}

void switchPreset(void)
{
  switch(preset)
  {
    case maxfunkyelectricguitar:
      param1_pulses = 0.0      /PRECISION_1; // Pre Gain
      param2_pulses = -30.0    /PRECISION_2; // Threshold
      param3_pulses = 4.0      /PRECISION_3; // Ratio
      param4_pulses = 53.0     /PRECISION_4; // Attack
      param5_pulses = 53.0     /PRECISION_5; // Hold
      param6_pulses = 500.0    /PRECISION_6; // Decay
      param7_pulses = 1.5      /PRECISION_7; // Bright
      param8_pulses = 0; // PrePost Bright
      param9_pulses = 0.0      /PRECISION_9; // Master Volume
      param10_pulses = 6.0     /PRECISION_10; // Post Gain
      param11_pulses = 500.0  /PRECISION_11; // Parametric Eq Frequency
      break;
    case andreaelectricbass:
      param1_pulses = 1.5      /PRECISION_1; // Pre Gain
      param2_pulses = -29.0    /PRECISION_2; // Threshold
      param3_pulses = 2.9      /PRECISION_3; // Ratio
      param4_pulses = 21.8     /PRECISION_4; // Attack
      param5_pulses = 53.0     /PRECISION_5; // Hold
      param6_pulses = 500.0    /PRECISION_6; // Decay
      param7_pulses = 1.5      /PRECISION_7; // Bright
      param8_pulses = 0; // PrePost Bright
      param9_pulses = 0.0      /PRECISION_9; // Master Volume
      param10_pulses = 9.0     /PRECISION_10; // Post Gain
      param11_pulses = 1000.0  /PRECISION_11; // Parametric Eq Frequency
      break;
    case andreaerocklectricguitar:
      param1_pulses = 1.5      /PRECISION_1; // Pre Gain
      param2_pulses = -33.0    /PRECISION_2; // Threshold
      param3_pulses = 3.0      /PRECISION_3; // Ratio
      param4_pulses = 53.0     /PRECISION_4; // Attack
      param5_pulses = 53.0     /PRECISION_5; // Hold
      param6_pulses = 209.0    /PRECISION_6; // Decay
      param7_pulses = 1.5      /PRECISION_7; // Bright
      param8_pulses = 0; // PrePost Bright
      param9_pulses = 0.0      /PRECISION_9; // Master Volume
      param10_pulses = 6.5     /PRECISION_10; // Post Gain
      param11_pulses = 1000.0  /PRECISION_11; // Parametric Eq Frequency
      break;
    case andreaacousticguitar:
      param1_pulses = 3.0      /PRECISION_1; // Pre Gain
      param2_pulses = -40.0    /PRECISION_2; // Threshold
      param3_pulses = 2.2      /PRECISION_3; // Ratio
      param4_pulses = 95.0     /PRECISION_4; // Attack
      param5_pulses = 53.0     /PRECISION_5; // Hold
      param6_pulses = 417.0    /PRECISION_6; // Decay
      param7_pulses = 0.0      /PRECISION_7; // Bright
      param8_pulses = 0; // PrePost Bright
      param9_pulses = 0.0      /PRECISION_9; // Master Volume
      param10_pulses = 8.8     /PRECISION_10; // Post Gain
      param11_pulses = 1000.0  /PRECISION_11; // Parametric Eq Frequency
      break;
  }
  
  setPulses(param1_pulses);
  set_regulation_precision(PRECISION_1);
  param1_value = processencoder(PREGAIN_MIN, PREGAIN_MAX); // Pre Gain
  
  setPulses(param2_pulses);
  set_regulation_precision(PRECISION_2);
  param2_value = processencoder(THR_MIN, THR_MAX); // Threshold
  
  setPulses(param3_pulses);
  set_regulation_precision(PRECISION_3);
  param3_value = processencoder(RATIO_MIN, RATIO_MAX); // Ratio
  
  setPulses(param4_pulses);
  set_regulation_precision(PRECISION_4);
  param4_value = processencoder(ATTACK_MIN, ATTACK_MAX); // Attack
  
  setPulses(param5_pulses);
  set_regulation_precision(PRECISION_5);
  param5_value = processencoder(HOLD_MIN, HOLD_MAX); // Hold
  
  setPulses(param6_pulses);
  set_regulation_precision(PRECISION_6);
  param6_value = processencoder(DECAY_MIN, DECAY_MAX); // Decay
  
  setPulses(param7_pulses);
  set_regulation_precision(PRECISION_7);
  param7_value = processencoder(BRIGHT_MIN, BRIGHT_MAX); // Bright
  
  param8_value = selectorwithencoder(param8_pulses, 1); // Bright Pre Post
  
  setPulses(param9_pulses);
  set_regulation_precision(PRECISION_9);
  param9_value = processencoder(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX); // Master Volume
  
  setPulses(param10_pulses);
  set_regulation_precision(PRECISION_10);
  param10_value = processencoder(POSTGAIN_MIN, POSTGAIN_MAX); // Post Gain
  
  setPulses(param11_pulses);
  set_regulation_precision(PRECISION_11);
  param11_value = processencoder(F_MIN, F_MAX); // Parametric Eq Frequency
  
  // Update values into DSP registers
  pregain = DB_TO_LINEAR(param1_value);
  
  // Bright Filter Pre & Post
  setBrightPrePost(param8_value);
  
  cmp1.threshold = param2_value;
  cmp1.ratio = param3_value;
  cmp1.attack = param4_value;
  cmp1.hold = param5_value;
  cmp1.decay = param6_value;
  cmp1.postgain = param10_value;
  
  // Notch Eq 
  setNotchEq(param11_value);
  
  // Master Volume
  postgain = DB_TO_LINEAR(param9_value);
  
  setPulses(param1_pulses);
  func_counter = 0;
}

void changeValue(void)
{
    if(old_func_counter != func_counter)
    {
      restore = 1;
      old_func_counter = func_counter;
    }
    switch(func_counter)
    {
    case 0: // Pre Gain
      if(restore)
      {
        restore = 0;
        setPulses(param1_pulses);
      }
      param1_pulses = getPulses();
      set_regulation_precision(PRECISION_1);
      param1_value = processencoder(PREGAIN_MIN, PREGAIN_MAX); // Pre Gain
      pregain = DB_TO_LINEAR(param1_value);
      break;
    case 1: // Threshold
      if(restore)
      {
        restore = 0;
        setPulses(param2_pulses);
      }
      param2_pulses = getPulses();
      set_regulation_precision(PRECISION_2);
      param2_value = processencoder(THR_MIN, THR_MAX); // Threshold
      cmp1.threshold = param2_value;
      break;
    case 2: // Ratio
      if(restore)
      {
        restore = 0;
        setPulses(param3_pulses);
      }
      param3_pulses = getPulses();
      set_regulation_precision(PRECISION_3);
      param3_value = processencoder(RATIO_MIN, RATIO_MAX); // Ratio
      cmp1.ratio = param3_value;
      break;
    case 3: // Attack
      if(restore)
      {
        restore = 0;
        setPulses(param4_pulses);
      }
      param4_pulses = getPulses();
      set_regulation_precision(PRECISION_4);
      param4_value = processencoder(ATTACK_MIN, ATTACK_MAX); // Attack
      cmp1.attack = param4_value;
      break;
    case 4: // Hold
      if(restore)
      {
        restore = 0;
        setPulses(param5_pulses);
      }
      param5_pulses = getPulses();
      set_regulation_precision(PRECISION_5);
      param5_value = processencoder(HOLD_MIN, HOLD_MAX); // Hold
      cmp1.hold = param5_value;
      break;
    case 5: // Decay
      if(restore)
      {
        restore = 0;
        setPulses(param6_pulses);
      }
      param6_pulses = getPulses();
      set_regulation_precision(PRECISION_6);
      param6_value = processencoder(DECAY_MIN, DECAY_MAX); // Decay
      cmp1.decay = param6_value;
      break;  
    case 6: // Bright
      if(restore)
      {
        restore = 0;
        setPulses(param7_pulses);
      }
      param7_pulses = getPulses();
      set_regulation_precision(PRECISION_7);
      param7_value = processencoder(BRIGHT_MIN, BRIGHT_MAX); // Bright
      setBrightPrePost(param8_value);
      break;
    case 7: // Bright Pre / Post
      if(restore)
      {
        restore = 0;
        setPulses(param8_pulses);
      }
      param8_pulses = getPulses();
      param8_value = selectorwithencoder(param8_pulses, 1); // Bright Pre Post
      setBrightPrePost(param8_value);
      break;
    case 8: // Master Volume
      if(restore)
      {
        restore = 0;
        setPulses(param8_pulses);
      }
      param8_pulses = getPulses();
      set_regulation_precision(PRECISION_9);
      param9_value = processencoder(MASTER_VOLUME_MIN, MASTER_VOLUME_MAX); // Master Volume
      postgain = DB_TO_LINEAR(param9_value);
      break;
    case 9: // Post Gain
      if(restore)
      {
        restore = 0;
        setPulses(param10_pulses);
      }
      param10_pulses = getPulses();
      set_regulation_precision(PRECISION_10);
      param10_value = processencoder(POSTGAIN_MIN, POSTGAIN_MAX); // Post Gain
      cmp1.postgain = param10_value;
      break;
    case 10: // Parametric Eq Frequency
      if(restore)
      {
        restore = 0;
        setPulses(param11_pulses);
      }
      param11_pulses = getPulses();
      set_regulation_precision(PRECISION_11);
      param11_value = processencoder(F_MIN, F_MAX); // Parametric Eq Frequency
      setNotchEq(param11_value);
      break;
    } // End switch func_counter
}

void setNotchEq(float f)
{
  notch.Q = 1.41;
  notch.gain = 0.0;
  notch.f = f;
  notch.boost = -6.00;
  notch.type = Parametric;
  notch.phase = false;
  if(preset==andreaacousticguitar)
    notch.onoff = true;
  else
    notch.onoff = false;
  EQ2ndOrd(&notchcoeff, &notch);
}

void setBrightPrePost(uint8_t value)
{
  pre.S = 1.0;
  post.S = 1.0;
  pre.gain = 0.0;
  post.gain = 0.0;
  switch(preset)
  {
    case maxfunkyelectricguitar:
      pre.f = 2000.0; // Guitar
      post.f = 2000.0;
      break;
    case andreaelectricbass:
      pre.f = 700.0; // Bass
      pre.f = 700.0;
      break;
    case andreaerocklectricguitar:
      pre.f = 2000.0; // Guitar
      pre.f = 2000.0;
      break;
    case andreaacousticguitar:
      pre.f = 850.0; // Acoustic Guitar
      pre.f = 850.0;
      break;  
  }
  pre.boost = param7_value;
  post.boost = param7_value;
  pre.type = HighShelf;
  post.type = HighShelf;
  pre.phase = false;
  post.phase = false;
  
  if(value==1) // Pre
  {
    pre.onoff = true;
    post.onoff = false;
  }
  else if(value==2) // Post
  {
    pre.onoff = false;
    post.onoff = true;
  }
  
  EQ2ndOrd(&precoeff, &pre);
  EQ2ndOrd(&postcoeff, &post);
}

int main()
{
    platformInit(myKnobConfig);

    printf("Starting Aida DSP mini...\n");

    // Initial FX setup
    preset = maxfunkyelectricguitar;
    switchPreset();
    
    platformRegisterUserCallback(userCallback);
    platformRegisterSwitchCallback(switchCallback);
    platformRegisterIdleCallback(idleCallback);
    codecRegisterProcessFunction(process);

    platformMainloop();

    return 0;
}
