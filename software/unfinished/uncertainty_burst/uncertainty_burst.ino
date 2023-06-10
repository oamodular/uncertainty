#include <Arduino.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "fp.hpp"

using namespace fp;

// ---------------------------------------- //
// ----------- TODO: EVERYTHING ----------- //
// ---------------------------------------- //

// should gates be toggled on clock, or set to the dice roll on clock?
#define TOGGLE_GATES false

// number of gates
#define NUM_GATES 8

typedef fp_t<int, 12> cv_t;

// hold pins for gates
int gatePins[] = {27,28,29,0,3,4,2,1};
// gate states
int gateStates[] = {0,0,0,0,0,0,0,0};
// gate probabilities
cv_t gateProbabilities[] = {cv_t(95/100.0),cv_t(90/100.0),cv_t(75/100.0),cv_t(50/100.0),cv_t(25/100.0),cv_t(10/100.0),cv_t(5/100.0),cv_t(2.5/100.0)};
// inv gate probabilities
cv_t invGateProbabilitiesMax[] = {cv_t(99.5/100.0),cv_t(80/100.0),cv_t(50/100.0),cv_t(20/100.0)};
cv_t invGateProbabilitiesMin[] = {cv_t(60/100.0),cv_t(40/100.0),cv_t(10/100.0),cv_t(0.5/100.0)};
// ADC input pin
int inputPin = 26;
// ADC threshold from low to high (tunable)
cv_t lowToHighInputThreshold = cv_t(1.0);
// ADC threshold from high to low (tunable)
cv_t highToLowInputThreshold = cv_t(0.5);
// last ADC value read, used to detect rising edge
cv_t lastInput = cv_t(0.0);
// bool to track input state for debounce/hysteresis
bool isInputHigh = false;
// bool to track if output has been triggered negatively
bool negTriggered = false;
// int to track how many samples we've accumulated
int samplesAccumulated = 0;
// frac to track max value
cv_t maxValue = 0;

uint32_t startupCounter = 0;

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
  }

  Serial.begin(9600);

  // startup sequence
  while(startupCounter<128*8) {
    int pinIndex = startupCounter>>7;
    for(int i=0;i<8;i++) {
      if(i==pinIndex) {
        gpio_put(gatePins[pinIndex], startupCounter%128 < 120);
      } else {
        gpio_put(gatePins[i], 0);
      }
      gateStates[i] = 0;
    }
    startupCounter++;
    delay(1);
    // process inputs here first
    return;
  }
}

class AnalogSchmiTrig {
public:
  int samplesAccumulated = 0;
  bool isInputHigh = false;
  bool triggered = false;
  bool polarity = true;
  cv_t lowToHighInputThreshold = cv_t(1.0);
  cv_t highToLowInputThreshold = cv_t(0.5);
  cv_t lastInput = cv_t(0.0);
  cv_t maxValue = cv_t(0.0);
  
  void Process(cv_t input) {
    bool polarity = input > cv_t(0.0) ? true : false;
    input = polarity ? input : -input;
    
    if(isInputHigh == false && lastInput < lowToHighInputThreshold && input > lowToHighInputThreshold) {
      isInputHigh = true;
    }
    if(isInputHigh == true && lastInput > highToLowInputThreshold && input < highToLowInputThreshold) {
      isInputHigh = false;
      maxValue = 0;
    }
    
    if(triggered) {
      if(samplesAccumulated < 4) {
        if(!polarity) {
          maxValue = max(maxValue, min(cv_t(1.0), cv_t((max(input, cv_t(1.0))-cv_t(1.0))*cv_t(0.3))));
        }
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

void loop() {
  cv_t input = (cv_t(adc_read())>>12)*cv_t(10.0) - cv_t(5.0);
  
  for(int i=0; i<NUM_GATES; i++) {
    gpio_put(gatePins[i], 0);
  }

  /*
  // poll ADC as fast as possible
  cv_t input = (cv_t(adc_read())>>12)*cv_t(10.0) - cv_t(5.0);
  bool polarity = input > cv_t(0.0) ? true : false;
  input = polarity ? input : -input;

  // if there's a rising edge...
  if(lastInput < lowToHighInputThreshold && input > lowToHighInputThreshold && isInputHigh == false) {
    isInputHigh = true;
    // if positive gate
    if(polarity) {
      // iterate over the gates
      for(int i=0; i<NUM_GATES; i++) {

        
        // set the gpio pin
        gpio_put(gatePins[i], gateStates[i]);
      }
    }
    else // if negative gate
    {
      negTriggered = true;
    }
  }

  if(negTriggered) {
    if(samplesAccumulated < 4) {
      if(!polarity) {
        maxValue = max(maxValue, min(cv_t(1.0), cv_t((max(input, cv_t(1.0))-cv_t(1.0))*cv_t(0.3))));
      }
      samplesAccumulated++;
    } else {
      // iterate over the gates
      for(int i=0; i<NUM_GATES/2; i++) {
        
        // set the gpio pins
        gpio_put(gatePins[i*2], gateStates[i]);
        gpio_put(gatePins[i*2+1], gateStates[i] ? 0 : 1);
      }

      maxValue = 0;
      samplesAccumulated = 0;
      negTriggered = false;
    }
  }

  // turn off gates when input goes low
  if(lastInput > highToLowInputThreshold && input < highToLowInputThreshold && isInputHigh == true) {
    isInputHigh = false;
    for(int i=0; i<NUM_GATES; i++) {
      gpio_put(gatePins[i], 0);
    }
  }
  
  // record the current input for the next calc
  lastInput = input;
  */
}
