import time
import board
import digitalio
from analogio import AnalogIn

class Uncertainty:

    def __init__(self):
        self.adc = AnalogIn(board.A0)
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

        self.high_thresh = 50000 # just under 3v
        self.low_thresh = 40000 # around 1v
        self.current_gate = 0

    def isRisingEdge(self):
        if(self.adc.value > self.high_thresh and self.current_gate == 0):
            self.current_gate = 1
            return 1
        else:
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
