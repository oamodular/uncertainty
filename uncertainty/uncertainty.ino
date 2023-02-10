#include <Arduino.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "fp.hpp"

using namespace fp;

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
cv_t gateProbabilities[] = {cv_t(99/100.0),cv_t(91/100.0),cv_t(67/100.0),cv_t(50/100.0),cv_t(33/100.0),cv_t(13/100.0),cv_t(5/100.0),cv_t(1/100.0)};
// inv gate probabilities
cv_t invGateProbabilitiesMax[] = {cv_t(99.5/100.0),cv_t(80/100.0),cv_t(50/100.0),cv_t(20/100.0)};
cv_t invGateProbabilitiesMin[] = {cv_t(20/100.0),cv_t(50/100.0),cv_t(80/100.0),cv_t(99.5/100.0)};
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
}

void loop() {
  // poll ADC as fast as possible
  cv_t input = (cv_t(adc_read())>>12)*cv_t(10.0) - cv_t(5.0);
  bool polarity = input > cv_t(0.0) ? true : false;
  input = polarity ? input : -input;

  // startup sequence
  if(startupCounter<128*8) {
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
    lastInput = input;
    return;
  }

  // if there's a rising edge...
  if(lastInput < lowToHighInputThreshold && input > lowToHighInputThreshold && isInputHigh == false) {
    isInputHigh = true;
    // if positive gate
    if(polarity) {
      // iterate over the gates
      for(int i=0; i<NUM_GATES; i++) {
        // wrap random number generation
        // to a max of roughly 1000 
        cv_t diceRoll = cv_t(rand() & 0x3FFF)>>14;

        // modify this #define at the top of the
        // file to change how the module works
        if(TOGGLE_GATES) {
          // toggle gate states according to probability
          if(diceRoll < gateProbabilities[i]) {
            // if gate state is 0, set it to 1
            // if gate state is 1, set it to 0
            gateStates[i] = gateStates[i] == 0 ? 1 : 0;
          }
        } else {
          // set gate states to result of dice roll
          gateStates[i] = diceRoll < gateProbabilities[i] ? 1 : 0;
        }
        
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
        // wrap random number generation
        // to a max of roughly 1000 
        cv_t diceRoll = cv_t(rand() & 0x3FFF)>>14;

        // modify this #define at the top of the
        // file to change how the module works
        if(TOGGLE_GATES) {
          // toggle gate states according to probability
          if(diceRoll < (cv_t(invGateProbabilitiesMax[i]*maxValue) + cv_t(invGateProbabilitiesMin[i]*(cv_t(1)-maxValue)))) {
            // if gate state is 0, set it to 1
            // if gate state is 1, set it to 0
            gateStates[i] = gateStates[i] == 0 ? 1 : 0;
          }
        } else {
          // set gate states to result of dice roll
          gateStates[i] = diceRoll < (cv_t(invGateProbabilitiesMax[i]*maxValue) + cv_t(invGateProbabilitiesMin[i]*(cv_t(1)-maxValue))) ? 1 : 0;
        }
        
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
}
