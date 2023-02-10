#include <Arduino.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define NUM_OUTPUTS 8 // number of gates
#define DSIG_BITS 16 // bit precision
int outputPins[] = {27,28,29,0,3,4,2,1}; // hold pins for gates
int32_t channelError[] = {0,0,0,0,0,0,0,0};
int32_t channelTarget[] = {1000,2000,4000,8000,16000,32000,0,0};

static bool dsigHandler(struct repeating_timer *t) {
  for(int i=0;i<NUM_OUTPUTS;i++) {
    bool out = channelTarget[i] > channelError[i] ? 1 : 0;
    channelError[i] += (out ? (1<<DSIG_BITS) : 0) - channelTarget[i];
    gpio_put(outputPins[i], out);
  }
  return true;
}

static void core1Entry() {
  struct repeating_timer _timer_;
  // initialize gate out pins
  for(int i=0; i<NUM_OUTPUTS; i++) {
    int pin = outputPins[i];
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
  }
  add_repeating_timer_us(-6, dsigHandler, NULL, &_timer_);
  while(1) {
    if(multicore_fifo_rvalid()) {
      uint32_t val = multicore_fifo_pop_blocking();
      channelTarget[val>>24] = val & 0x00FFFFFF;
    }
  }
}
  
void setup() {
  // 2x overclock for MAX POWER
  set_sys_clock_khz(250000, true);
  multicore_launch_core1(core1Entry);
}

float phase = 0;
void loop() {
  for(int i=0;i<NUM_OUTPUTS;i++) {
    uint32_t outVal = (uint32_t)((sin(phase+(0.4*i))*0.5+0.5)*(1<<DSIG_BITS));
    multicore_fifo_push_blocking((outVal&0x00FFFFFF) | (i<<24));
    delay(1);
  }
  phase += 0.02;
}
