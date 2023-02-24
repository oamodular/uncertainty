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

// evenly spaced burst generator
class EvenBurst {
public:
  Trigger trig;
  int bursts;
  int maxBursts;
  int spacing;
  int output;
  int phase;
  EvenBurst(int output=0, int maxBursts=3, int spacingInMs=50) : trig(output) {
    this->bursts = maxBursts;
    this->maxBursts = maxBursts;
    this->spacing = spacingInMs;
    this->output = output;
    this->phase = 0;
  }
  void Reset() {
    bursts = 1;
    phase = 0;
    trig.Reset();
  }
  void Process() {
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

AnalogSchmitTrig analogTrig;
EvenBurst* bursts[NUM_GATES];

// audio rate callback- meat of the program goes here
static bool audioHandler(struct repeating_timer *t) {
  cv_t input = (cv_t(adc_read())>>11)*cv_t(5.0) - cv_t(5.0);
  analogTrig.Process(input);
  if(analogTrig.Triggered()) {
    Serial.println(float(analogTrig.Value()));
    cv_t spacingCoef = (input * cv_t(1.0 / 4.42));
    spacingCoef = cv_t(1) - spacingCoef;
    spacingCoef = spacingCoef * spacingCoef;
    for(int i=0; i<NUM_GATES; i++) {
      bursts[i]->spacing = int(spacingCoef * fp_t<int, 0>(400))+1;
      bursts[i]->Reset();
    }
  }
  for(int i=0; i<NUM_GATES; i++) {
    bursts[i]->Process();
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
    bursts[i] = new EvenBurst(i, i+1, 100);
  }

  Serial.begin(9600);

  startupSequence();

  add_repeating_timer_us(-TIMER_INTERVAL, audioHandler, NULL, &_timer_);
}

void loop() {

}
