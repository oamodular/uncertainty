import time
from machine import Pin, ADC

cv_in = ADC(Pin(26))
outs = [Pin(27, Pin.OUT),
        Pin(28, Pin.OUT),
        Pin(29, Pin.OUT),
        Pin(0, Pin.OUT),
        Pin(3, Pin.OUT),
        Pin(4, Pin.OUT),
        Pin(2, Pin.OUT),
        Pin(1, Pin.OUT)]

while True:

    active_gate = int(cv_in.read_u16() / 4096) - 8

    outs[active_gate].value(1)

    for num in range (8):
        if num != active_gate:
            outs[num].value(0)

    time.sleep(0.001)
