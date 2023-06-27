#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

#include "fp.hpp"
#include "dsp.h"

using namespace fp;

#define TIMER_INTERVAL ((int)(1000000/SAMPLE_RATE))

const byte PICO_I2C_ADDRESS = 55;
const byte PICO_I2C_SDA = 6;
const byte PICO_I2C_SCL = 7;

// hold pins for gates
int gatePins[] = {27,28,29,0,3,4,2,1};
// ADC input pin
int inputPin = 26;

class AnalogOut {
public:
  TrigGen trig;
  Env env;
  Osc osc;
  SlewedDC dc;
  uint16_t res;
  uint pin;
  AnalogOut(int pin, int resolution = 127) {
    this->pin = pin;
    this->res = resolution;
    gpio_set_function(pin, GPIO_FUNC_PWM);
    if(pin != 29 && pin != 1 && pin != 2) {
      uint slice_num = pwm_gpio_to_slice_num(pin);
      pwm_config cfg = pwm_get_default_config();
      pwm_config_set_clkdiv_int(&cfg, 1);
      pwm_config_set_wrap(&cfg, this->res);
      pwm_init(slice_num, &cfg, true);
    }
    pwm_set_gpio_level(pin, 0);
  }
  void Process() {
    int oscOut = osc.delta == 0 ? 127 : osc.Process();
    int envTrigDC = max(max(dc.Process(), trig.Process()), env.Process());
    pwm_set_gpio_level(pin, (oscOut * envTrigDC) >> 7);
  }
};

struct {
  uint8_t command;
  uint16_t values[3];
} i2cdata;

typedef enum {
  CMD_PULSE,
  CMD_SET,
  CMD_ENV,
  CMD_LFO,
  CMD_OSC,
  CMD_LAST
} command_t;

AnalogOut* outputs[8];

uint64_t diff;

void i2c_receive(int bytes_count) {     // bytes_count gives number of bytes in rx buffer    
  if(bytes_count) i2cdata.command = Wire1.read();
  for(int i=0; i<bytes_count - 1; i++) {
    int valueIndex = i / 2;
    if(i % 2 == 0) {
      i2cdata.values[valueIndex] = Wire1.read() << 8;
    } else {
      i2cdata.values[valueIndex] |= Wire1.read();
    }
  }
  
  int commandType = i2cdata.command / 8;
  int commandOutput = i2cdata.command % 8;
  int numParams = max(0, (bytes_count - 1) / 2);

  switch(commandType) {
    case CMD_PULSE:
      outputs[commandOutput]->trig.Reset();
      if(numParams > 0) outputs[commandOutput]->trig.width = i2cdata.values[0] * MS_DELTA;
      if(numParams > 1) outputs[commandOutput]->trig.amp = i2cdata.values[1];
      break;
    case CMD_SET:
      if(numParams > 0) outputs[commandOutput]->dc.Set(i2cdata.values[0]);
      if(numParams > 1) outputs[commandOutput]->dc.SetSlew(i2cdata.values[1]);
      break;
    case CMD_ENV:
      if(numParams > 0) outputs[commandOutput]->env.attack.SetDuration(i2cdata.values[0]);
      if(numParams > 1) outputs[commandOutput]->env.decay.SetDuration(i2cdata.values[1]);
      outputs[commandOutput]->env.Reset();
      break;
    case CMD_LFO:
      if(numParams < 1) outputs[commandOutput]->osc.phase = 0;
      if(numParams > 0) outputs[commandOutput]->osc.SetDuration(i2cdata.values[0]);
      if(numParams > 1) outputs[commandOutput]->osc.type = i2cdata.values[1];
      break;
    case CMD_OSC:
      if(numParams < 1) outputs[commandOutput]->osc.phase = 0;
      if(numParams == 1) outputs[commandOutput]->osc.SetNote(i2cdata.values[0]);
      if(numParams == 2) {
        outputs[commandOutput]->osc.SetNote(i2cdata.values[0]);
        outputs[commandOutput]->osc.type = i2cdata.values[1];
      }
      if(numParams == 3) outputs[commandOutput]->osc.SetNote(i2cdata.values[0], i2cdata.values[1], i2cdata.values[2]);
      break;
    default:
      break;
  }
}
void i2c_transmit() {
  uint8_t data[2] = {0, 0};
  int16_t in = ((adc_read() - (1<<11))<<4) - 925; // twiddle factor because our zero is a bit off
  data[0] = (in & 0xFF00) >> 8;
  data[1] = in & 0x00FF;
  Wire1.write(data, 2);
}

static bool audioHandler(struct repeating_timer *t) {
  for(int i=0; i<8; i++) outputs[i]->Process();
  return true;
}

void setup() {  
  // initialize ADC
  adc_init();
  adc_gpio_init(inputPin);
  adc_select_input(0);
  gpio_set_pulls(inputPin, false, false);

  // initialize gate out pins
  for(int i=0; i<8; i++) {
    outputs[i] = new AnalogOut(gatePins[i], 127);
  }

  Serial.begin(9600);
  
  Wire1.setSDA(6);
  Wire1.setSCL(7);
  Wire1.begin(PICO_I2C_ADDRESS);    // join i2c bus as slave
  Wire1.onReceive(i2c_receive);     // i2c interrupt receive
  Wire1.onRequest(i2c_transmit);    // i2c interrupt send
}

void loop() {
  absolute_time_t timeOfNextLoop = make_timeout_time_us(TIMER_INTERVAL);
  audioHandler(nullptr);
  //diff = absolute_time_diff_us(get_absolute_time(), timeOfNextLoop); // profiling to show how many us of processing wiggle room we have (last measured 15-16, occasionally 6)
  busy_wait_until(timeOfNextLoop);
}
