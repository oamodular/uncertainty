#include <Arduino.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "fp.hpp"

using namespace fp;

#define TIMER_INTERVAL ((int)(1000000.0/40000.0))
#define SAMPLE_RATE (1000000.0/TIMER_INTERVAL)

// number of gates
#define NUM_GATES 8

typedef fp_t<int, 12> cv_t;

// hold pins for gates
int gatePins[] = {27,28,29,0,3,4,2,1};
// ADC input pin
int inputPin = 26;

uint32_t startupCounter = 0;

struct repeating_timer _timer_;

// gate detector for analog input
class AnalogSchmitTrig {
public:
  int samplesAccumulated = 0;
  bool isInputHigh = false;
  bool triggered = false;
  bool fallen = false;
  bool polarity = true;
  cv_t lowToHighInputThreshold = cv_t(1.0);
  cv_t highToLowInputThreshold = cv_t(0.5);
  cv_t maxValue = cv_t(0.0);
  
  void Process(cv_t input) {
    polarity = input > cv_t(0.0) ? true : false;
    input = polarity ? input : -input;
    
    if(isInputHigh == false && input > lowToHighInputThreshold) {
      isInputHigh = true;
      triggered = true;
    }
    if(isInputHigh == true && input < highToLowInputThreshold) {
      isInputHigh = false;
      maxValue = 0;
      fallen = true;
    }
    
    if(triggered) {
      if(samplesAccumulated < 4) {
        maxValue = max(maxValue, input);
        samplesAccumulated++;
      }
    }
  }

  bool Triggered() {
    if(triggered && samplesAccumulated >= 4) {
      triggered = false;
      samplesAccumulated = 0;
      return true;
    }
    return false;
  }

  bool HasFallen() {
    bool res = fallen;
    if(res) fallen = false;
    return res;
  }

  bool IsHigh() { return isInputHigh; }

  cv_t Value() { return maxValue; }
};

// trigger generator
class Trigger {
public:
  int phase;
  int output;
  int width;
  Trigger(int output=0, int widthInSamples=100) {
    this->output = output;
    this->width = widthInSamples;
    this->phase = widthInSamples+1;
  }
  void Reset() {
    phase = 0;
  }
  bool Done() {
    return phase > width;
  }
  void Process() {
    gpio_put(gatePins[output], phase < width ? 1 : 0);
    if(phase <= width) phase++;
  }
};

class Burst {
public:
  Trigger trig;
  int bursts;
  int maxBursts;
  int spacing;
  int output;
  int phase;
  Burst(int output) : trig(output) {
    this->output = output;
    this->phase = 0;
  }
  virtual void Reset(cv_t input = 0) {}
  virtual void Process(cv_t input = 0) {}
  virtual void End(cv_t input = 0) {}
};

// evenly spaced burst generator
class EvenBurst : public Burst {
public:
  EvenBurst(int output=0, int maxBursts=3, int spacingInMs=50) : Burst(output) {
    this->bursts = maxBursts;
    this->maxBursts = maxBursts;
    this->spacing = spacingInMs;
  }
  void Reset(cv_t input = 0) {
    bursts = 1;
    phase = 0;
    trig.Reset();
  }
  void Process(cv_t input = 0) {
    if(bursts < maxBursts) {
      if(trig.Done() && phase > int(SAMPLE_RATE / 1000) * spacing) {
        bursts++;
        trig.Reset();
        phase = 0;
      }
      phase++;
    }
    trig.Process();
  }
};

// evenly spaced continuous burst generator
class ContinuousBurst : public Burst {
public:
  int spacingCoef = 1;
  bool firing = false;
  ContinuousBurst(int output=0, int maxBursts=3, int spacingInMs=50) : Burst(output) {
    this->spacingCoef = maxBursts;
  }
  void Reset(cv_t input = 0) {
    phase = 0;
    trig.Reset();
    firing = true;
  }
  void Process(cv_t input = 0) {
    if(firing) {
      if(input > cv_t(0)) {
        spacing = int(input * fp_t<int, 0>(400));
      }
      if(trig.Done() && phase > int(SAMPLE_RATE / 1000) * spacing * spacingCoef) {
        trig.Reset();
        phase = 0;
      }
      phase++;
    }
    trig.Process();
  }
  virtual void End(cv_t input = 0) { firing = false; }
};

AnalogSchmitTrig analogTrig;
Burst* bursts[NUM_GATES];

// audio rate callback- meat of the program goes here
static bool audioHandler(struct repeating_timer *t) {
  cv_t input = (cv_t(adc_read())>>11)*cv_t(5.0) - cv_t(5.0);
  cv_t spacingCoef = (((input > cv_t(0) ? input : -input) - cv_t(1)) * cv_t(1.0 / 4.42));
  if(spacingCoef < cv_t(0)) spacingCoef = cv_t(0);
  spacingCoef *= spacingCoef;
  //spacingCoef = cv_t(1) - spacingCoef;
  //spacingCoef = spacingCoef * spacingCoef;
  analogTrig.Process(input);
  if(analogTrig.Triggered()) {
    for(int i=0; i<NUM_GATES; i++) {
      bursts[i]->spacing = int(spacingCoef * fp_t<int, 0>(400))+5;
      bursts[i]->Reset();
    }
  }
  if(analogTrig.HasFallen()) {
    for(int i=0; i<NUM_GATES; i++) {
      bursts[i]->End();
    }
  }
  for(int i=0; i<NUM_GATES; i++) {
    bursts[i]->Process(spacingCoef);
  }
  return true;
}

// cute little led animation
void startupSequence() {
  // startup sequence
  while(startupCounter<128*8) {
    int pinIndex = startupCounter>>7;
    for(int i=0;i<8;i++) {
      if(i==pinIndex) {
        gpio_put(gatePins[pinIndex], startupCounter%128 < 120);
      } else {
        gpio_put(gatePins[i], 0);
      }
    }
    startupCounter++;
    delay(1);
  }
}

void setup() {
  // 2x overclock for MAX POWER
  set_sys_clock_khz(250000, true);
  
  // initialize ADC
  adc_init();
  adc_gpio_init(inputPin);
  adc_select_input(0);
  
  // initialize gate out pins
  for(int i=0; i<NUM_GATES; i++) {
    int pin = gatePins[i];
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    bursts[i] = (Burst*)(new ContinuousBurst(i, i+1, 100));
  }

  Serial.begin(9600);

  startupSequence();

  add_repeating_timer_us(-TIMER_INTERVAL, audioHandler, NULL, &_timer_);
}

void loop() {

}
