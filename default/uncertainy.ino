#include <Arduino.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

// should gates be toggled on clock, or set to the dice roll on clock?
#define TOGGLE_GATES false

// number of gates
#define NUM_GATES 8

// ADC helper values
#define ADC_0V (1<<11)
#define ADC_1V_INC ((1<<12)/10)

// hold pins for gates
int gatePins[] = {27,28,29,0,3,4,2,1};
// gate states
int gateStates[] = {0,0,0,0,0,0,0,0};
// gate probabilities
int gateProbabilities[] = {950,900,750,500,250,100,50,25};
// ADC input pin
int inputPin = 26;
// ADC threshold from low to high (tunable)
// -5v is 0, 0v is 2^11, 5v is 2^12
int lowToHighInputThreshold = ADC_0V + ADC_1V_INC*2;
// ADC threshold from high to low (tunable)
int highToLowInputThreshold = ADC_0V + ADC_1V_INC;
// last ADC value read, used to detect rising edge
int lastInput = ADC_0V;
// bool to track input state for debounce/hysteresis
bool isInputHigh = false;

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
  int input = adc_read();
  // if there's a rising edge...
  if(lastInput < lowToHighInputThreshold && input > lowToHighInputThreshold && isInputHigh == false) {
    isInputHigh = true;
    // iterate over the gates
    for(int i=0; i<NUM_GATES; i++) {
      // wrap random number generation
      // to a max of roughly 1000 
      int diceRoll = rand() & 0x03FF;

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
