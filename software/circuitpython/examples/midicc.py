ctl_nums = [0, 1, 2, 3, 4, 5, 6, 7]


import time
import board
import pwmio
import digitalio
from analogio import AnalogIn
import usb_midi
import adafruit_midi
from adafruit_midi.control_change import ControlChange

pwm_freq = 1000000

cv_in = AnalogIn(board.A0)
gates = [
    pwmio.PWMOut(board.D1, frequency=pwm_freq, duty_cycle=0),
    pwmio.PWMOut(board.D2, frequency=pwm_freq, duty_cycle=0),
    pwmio.PWMOut(board.D3, frequency=pwm_freq, duty_cycle=0),
    pwmio.PWMOut(board.D6, frequency=pwm_freq, duty_cycle=0),
    pwmio.PWMOut(board.D10, frequency=pwm_freq, duty_cycle=0),
    pwmio.PWMOut(board.D9, frequency=pwm_freq, duty_cycle=0),
    pwmio.PWMOut(board.D8, frequency=pwm_freq, duty_cycle=0),
    pwmio.PWMOut(board.D7, frequency=pwm_freq, duty_cycle=0),
]

midi = adafruit_midi.MIDI(
    midi_in=usb_midi.ports[0],
    midi_out=usb_midi.ports[1],
    in_channel=0,
    out_channel=0,
    debug=False,
)

old_cv = 0
last_cc_val = 0

def output_ctl(ctl, val):
    for x in range(0, len(ctl_nums)):
        if ctl_nums[x] == ctl:
            gates[x].duty_cycle = val * 512 # scale the 7-bit midi up to a 16-bit num
            return

while True:

    msg = midi.receive()
    if msg is not None:
        if isinstance(msg, ControlChange):
            if msg.control in ctl_nums:
                output_ctl(msg.control, msg.value)

    if abs(old_cv - cv_in.value) > 256:
        cv_raw = max(cv_in.value, 5900)
        cv_raw = min(cv_raw, 64300)

        usable_rangle = (64300 - 5900)
        ctl_val = int((((cv_raw - 5900) * 127) / usable_rangle) + 0)

        if ctl_val is not last_cc_val:
            cc = ControlChange(1, ctl_val)
            midi.send(cc)

        old_cv = cv_in.value
        last_cc_val = ctl_val
