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

    active_gate = abs(int((cv_in.read_u16() - 32768) / 4096))

    for num in range (8):
        if num != active_gate:
            outs[num].value(0)

    outs[active_gate].value(1)

    time.sleep(0.0001)
