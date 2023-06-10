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
// ADC input pin
int inputPin = 26;
// ADC threshold from low to high (tunable)
cv_t lowToHighInputThreshold = cv_t(2.0);
// ADC threshold from high to low (tunable)
cv_t highToLowInputThreshold = cv_t(1.0);
// last ADC value read, used to detect rising edge
cv_t lastInput = cv_t(0.0);
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

unsigned char state = 0;

void loop() {
  // poll ADC as fast as possible
  cv_t input = (cv_t(adc_read())>>12)*cv_t(10.0) - cv_t(5.0);
  bool polarity = input > cv_t(0.0) ? true : false;
  input = polarity ? input : -input;

  // if there's a rising edge...
  if(lastInput < lowToHighInputThreshold && input > lowToHighInputThreshold && isInputHigh == false) {
    isInputHigh = true;
    
    state += polarity ? 1 : -1;    
    
    // iterate over the gates
    for(int i=0; i<NUM_GATES; i++) {
      gpio_put(gatePins[i], (state>>i)&0x1);
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
