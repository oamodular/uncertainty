#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

// USB MIDI object
Adafruit_USBD_MIDI usb_midi;

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

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
  //set_sys_clock_khz(250000, true);

  #if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
    // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
    TinyUSB_Device_Init(0);
  #endif
  
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

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  // wait until device mounted
  while( !TinyUSBDevice.mounted() ) delay(1);
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  gpio_put(gatePins[(pitch+4)%8], 1);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  gpio_put(gatePins[(pitch+4)%8], 0);
}

void loop() {
  // poll ADC as fast as possible
  int input = adc_read();
  MIDI.read();
}
