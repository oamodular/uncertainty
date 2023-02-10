#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "fp.hpp"

// USB MIDI object
Adafruit_USBD_MIDI usb_midi;

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// number of gates
#define NUM_OUTPUTS 8
#define DSIG_BITS 14 // bit precision

// hold pins for gates
int outputPins[] = {27,28,29,0,3,4,2,1};
int32_t channelError[] = {0,0,0,0,0,0,0,0};
int32_t channelTarget[] = {0,0,0,0,0,0,0,0};

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

  #if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
    // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
    TinyUSB_Device_Init(0);
  #endif
  
  // initialize gate out pins
  for(int i=0; i<NUM_OUTPUTS; i++) {
    int pin = outputPins[i];
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
  }

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  // wait until device mounted
  while( !TinyUSBDevice.mounted() ) delay(1);

  multicore_launch_core1(core1Entry);
}

void sendNote(int note, int value) {
  uint32_t out = value == 127 ? uint32_t(1<<DSIG_BITS) : uint32_t((1<<(DSIG_BITS-7))*value);
  multicore_fifo_push_blocking((out&0x00FFFFFF) | (note<<24));
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  //gpio_put(outputPins[(pitch+4)%8], 1);
  sendNote((pitch+4)%8, velocity);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  sendNote((pitch+4)%8, 0);
}

void loop() {
  MIDI.read();
}
