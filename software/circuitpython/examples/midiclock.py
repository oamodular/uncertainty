import time
import math
import board
import digitalio
from analogio import AnalogIn
import usb_midi
import adafruit_midi
from adafruit_midi.timing_clock import TimingClock
from adafruit_midi.start import Start
from adafruit_midi.stop import Stop
from adafruit_midi.control_change import ControlChange


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

midi = adafruit_midi.MIDI(
    midi_in=usb_midi.ports[0],
    midi_out=usb_midi.ports[1],
    in_channel=0,
    out_channel=0,
    debug=False,
)

old_cv = 0
last_cc_val = 0

ticks = 0

while True:

    gates[0].value = False
    gates[2].value = False

    msg = midi.receive()

    if msg is not None:
        if isinstance(msg, TimingClock): # receive a MIDI tick message
            gates[2].value = True
            gates[3].value = (ticks%6 == 0)
            gates[4].value = (ticks%12 == 0)
            gates[5].value = (ticks%24 == 0)
            gates[6].value = (ticks%96 == 0)
            gates[7].value = (ticks == 0)
            ticks +=1
            if ticks == 384:
                ticks = 0
        if isinstance(msg, Start):
            gates[0].value = True
            gates[1].value = True
        if isinstance(msg, Stop):
            for gate in gates:
                gate.value = False


    # get the cv in from eurorack and send it as a midi controller
        # lets only do this stuff if the raw data has actually by a significant amount
    if abs(old_cv - cv_in.value) > 256:
        # convert the 16-bit incomving value to a 7-bit value for midi control changes
        cv_raw = max(cv_in.value, 5900)
        cv_raw = min(cv_raw, 64300)

        usable_rangle = (64300 - 5900)
        ctl_val = int((((cv_raw - 5900) * 127) / usable_rangle) + 0)

        if ctl_val is not last_cc_val:
            cc = ControlChange(1, ctl_val)
            midi.send(cc)

        # store old values for next cycle
        old_cv = cv_in.value
        last_cc_val = ctl_val
