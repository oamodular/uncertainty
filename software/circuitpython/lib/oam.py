import time
import board
import digitalio
from analogio import AnalogIn
import pwmio

pwm_freq = 1000000

class Uncertainty:

    def __init__(self, outputMode='digital'):
        self.adc = AnalogIn(board.A0)
        if (outputMode == 'digital'):

            self.outs = [
                digitalio.DigitalInOut(board.D1),
                digitalio.DigitalInOut(board.D2),
                digitalio.DigitalInOut(board.D3),
                digitalio.DigitalInOut(board.D6),
                digitalio.DigitalInOut(board.D10),
                digitalio.DigitalInOut(board.D9),
                digitalio.DigitalInOut(board.D8),
                digitalio.DigitalInOut(board.D7),
            ]
            for out in self.outs:
                out.direction = digitalio.Direction.OUTPUT
        elif (outputMode == 'pwm'):
            self.outs = [
                pwmio.PWMOut(board.D1, frequency=pwm_freq, duty_cycle=0),
                pwmio.PWMOut(board.D2, frequency=pwm_freq, duty_cycle=0),
                pwmio.PWMOut(board.D3, frequency=pwm_freq, duty_cycle=0),
                pwmio.PWMOut(board.D6, frequency=pwm_freq, duty_cycle=0),
                pwmio.PWMOut(board.D10, frequency=pwm_freq, duty_cycle=0),
                pwmio.PWMOut(board.D9, frequency=pwm_freq, duty_cycle=0),
                pwmio.PWMOut(board.D8, frequency=pwm_freq, duty_cycle=0),
                pwmio.PWMOut(board.D7, frequency=pwm_freq, duty_cycle=0),
            ]

        self.high_thresh = 50000 # just under 3v
        self.low_thresh = 40000 # around 1v
        self.current_gate = 0

    def isRisingEdge(self):
        isHigh = self.adc.value > self.high_thresh
        if(isHigh and self.current_gate == 0):
            self.current_gate = isHigh
            return 1
        else:
            self.current_gate = isHigh
            return 0

    def isFallingEdge(self):
        if(self.adc.value < self.low_thresh and self.current_gate == 1):
            self.current_gate = 0
            return 1
        else:
            return 0

    def isGateOn(self):
        if(self.adc.value > self.low_thresh):
            self.current_gate = 1
            return 1
        else:
            return 0

    def isGateOff(self):
        if(self.adc.value < self.low_thresh):
            self.current_gate = 0
            return 1
        else:
            return 0

    def lightsOut(self):
        for out in self.outs:
            out.value = 0
