#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <vector>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

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
  uint16_t res;
  uint pin;
  int dc;
  AnalogOut(int pin, int resolution = 127) {
    this->pin = pin;
    this->res = resolution;
    this->dc = 0;
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
  void Set(int x) {
    dc = x;
  }
  void Process() {
    pwm_set_gpio_level(pin, max(max(dc, trig.Process()), env.Process()));
  }
};

struct {
  uint8_t command;
  uint16_t values[3];
} i2cdata;

typedef enum {
  CMD_SET,
  CMD_PULSE,
  CMD_ENV,
  CMD_LAST
} command_t;

AnalogOut* outputs[8];

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
    case CMD_SET:
      outputs[commandOutput]->trig.Reset();
      break;
    case CMD_PULSE:
      outputs[commandOutput]->Set(i2cdata.values[0]);
      break;
    case CMD_ENV:
      if(numParams > 0) outputs[commandOutput]->env.attack.SetDuration(i2cdata.values[0]);
      if(numParams > 1) outputs[commandOutput]->env.decay.SetDuration(i2cdata.values[1]);
      outputs[commandOutput]->env.Reset();
      break;
    default:
      break;
  }
}
void i2c_transmit() {
  Wire1.write(adc_read());
}

static bool audioHandler(struct repeating_timer *t) {
  for(int i=0; i<8; i++) outputs[i]->Process();
  return true;
}

struct repeating_timer _timer_;
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
  
  Wire1.setSDA(6);
  Wire1.setSCL(7);
  Wire1.begin(PICO_I2C_ADDRESS);    // join i2c bus as slave
  Wire1.onReceive(i2c_receive);     // i2c interrupt receive
  Wire1.onRequest(i2c_transmit);    // i2c interrupt send

  add_repeating_timer_us(-TIMER_INTERVAL, audioHandler, NULL, &_timer_);
}

void loop() {
  delay(1);
}
