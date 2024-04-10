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
counter = 0

while True:

    counter += 1
    
    for num in range (8):
        val = (counter % (2 ** (num+1))) == 0
        outs[num].value(val)
        
    bpm = abs(int((cv_in.read_u16() - 32768) / 100))
    s_per_beat = 60 / bpm

    time.sleep(s_per_beat)