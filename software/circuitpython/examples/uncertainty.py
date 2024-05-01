prob = [99,91,67,50,33,13,5,1]

import time
import board
import digitalio
from analogio import AnalogIn
import random

cv_in = AnalogIn(board.A0)
gates = [
    digitalio.DigitalInOut(board.D1),
    digitalio.DigitalInOut(board.D2),
    digitalio.DigitalInOut(board.D3),
    digitalio.DigitalInOut(board.D6),
    digitalio.DigitalInOut(board.D10),
    digitalio.DigitalInOut(board.D9),
    digitalio.DigitalInOut(board.D8),
    digitalio.DigitalInOut(board.D7),
]

for gate in gates:
    gate.direction = digitalio.Direction.OUTPUT

high_thresh = 40000
low_thresh = 38000
current_gate = 0

while True:

    if cv_in.value < low_thresh:
        for gate in gates:
            gate.value = 0
        current_gate = 0
    elif cv_in.value > high_thresh and current_gate == 0:
        for i in range(8):
            flip = random.randrange(100) < prob[i]
            gates[i].value = flip
        current_gate = 1
