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

// number of gate outs
#define NUM_GATES 8

// audio/cv "real" numbers
typedef fp_t<int,12> cv_t;

// hold pins for gates
int gatePins[] = {27,28,29,0,3,4,2,1};
// ADC input pin
int inputPin = 26;
// last ADC value read, used to detect rising edge
cv_t lastInput = cv_t(0.0);

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

class DCFilter {
private:
  // we're doing filtering of a sort, so let's
  // use high precision values
  typedef fp_t<int, 26> avg_t;
  avg_t lastVal;
public:
  DCFilter() {
    lastVal = 0;
  }
  cv_t Process(cv_t in) {
    avg_t delta = in - lastVal;
    lastVal += delta>>13; // to modify, shift by more bits for slower response to DC/low-freq input or fewer for more aggressive filtering
    return in - cv_t(lastVal);
  }
};

class MeterBallistics {
private:
  // we're doing filtering of a sort, so let's
  // use high precision values
  typedef fp_t<int, 24> slew_t;
public:
  slew_t lastVal;
  MeterBallistics() {
    lastVal = 0;
  }
  cv_t Process(cv_t in) {
    // get magnitude of input
    in = in > slew_t(0) ? in : -in;
    // how much did the magnitude of the input value change since last time?
    slew_t delta = slew_t(in) - lastVal;
    // if we're decreasing, seriously limit how quickly we can decrease
    if(delta < slew_t(0)) {
      // really hacky but efficient divide by 4096 samples, which is like 100ms fall time
      lastVal += delta>>12; // modify to alter fall time, bigger number longer fall
    } else { // otherwise just adjust by the difference, mimicking the input
      lastVal += delta;
    }
    return cv_t(lastVal);
  }
};

// cute little led animation
void startupSequence() {
  uint32_t startupCounter = 0;
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

// classes for handling gate/trigger output
Trigger* triggers[NUM_GATES];
MeterBallistics meter;
DCFilter dcFilter;

cv_t debugVal = 0;

// audio rate callback- meat of the program goes here
static bool audioHandler(struct repeating_timer *t) {
  // poll ADC and convert from 12 bit value to [-1.0, 1.0]
  cv_t input = (cv_t(adc_read() - (1<<11)) >> 11) * cv_t(5.0 / 4.42) - cv_t(0.04);

  debugVal = input;

  // DC filter input
  input = dcFilter.Process(input);

  // meter processing
  input = meter.Process(input);

  // scale output levels to go from 0 to 8
  int level = int(input*fp_t<int,0>(8));

  // set outputs
  for(int i=0;i<8;i++) {
    if(i+1 <= level) triggers[i]->Reset();
    triggers[i]->Process();
  }

  // record the current input for the next calc
  lastInput = input;

  return true;
}

struct repeating_timer _timer_;

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
    triggers[i] = new Trigger(i, 500 /* modify/increase to hold gates open for longer */ );
  }

  // oooh pretty
  startupSequence();

  // audio callback
  add_repeating_timer_us(-TIMER_INTERVAL, audioHandler, NULL, &_timer_);

  // init serial debugging
  Serial.begin(9600);
}

void loop() {
  Serial.println(float(debugVal));
  delay(100);
}
