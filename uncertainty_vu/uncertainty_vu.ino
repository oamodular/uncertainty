#include <Arduino.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "fp.hpp"

using namespace fp;

// number of gates
#define NUM_GATES 8

typedef fp_t<int,12> cv_t;

// hold pins for gates
int gatePins[] = {27,28,29,0,3,4,2,1};
// ADC input pin
int inputPin = 26;
// last ADC value read, used to detect rising edge
cv_t lastInput = cv_t(0.0);

void setup() {
  // 2x overclock for MAX POWER
  //set_sys_clock_khz(250000, true);

  //Serial.begin(115200);
  
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
  cv_t input = cv_t(adc_read() - (1<<11));
  bool polarity = input > cv_t(0.0) ? true : false;
  input = polarity ? input : -input;

  // hacky slew limit
  input = cv_t(input>>10) + cv_t((lastInput>>10)*fp_t<int,0>(1023));

  // scale output levels
  int level = int((input>>11)*fp_t<int,0>(10));

  // set outputs
  for(int i=0;i<8;i++) gpio_put(gatePins[i], i+1 <= level);

  // record the current input for the next calc
  lastInput = input;

  /*
  Serial.printf("Input: %f\n", float(input));
  Serial.printf("Norml: %f\n", float(input>>11));
  Serial.printf("Intgr: %d\n", int((input>>11)*fp_t<int,0>(9)));

  delay(100);
  */
}
