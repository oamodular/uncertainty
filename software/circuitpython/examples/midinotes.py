midi_notes = [60, 61, 62, 63, 64, 65, 66, 67]


import time
import board
import digitalio
from analogio import AnalogIn
import usb_midi
import adafruit_midi
from adafruit_midi.note_on import NoteOn
from adafruit_midi.note_off import NoteOff
from adafruit_midi.control_change import ControlChange
from adafruit_midi.pitch_bend import PitchBend


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

def play_note(note, state):
    for x in range(0, len(midi_notes)):
        if midi_notes[x] == note:
            gates[x].value = state
            return

while True:

    #get the midi from usb and send it out as eurorack gates
    msg = midi.receive()
    if msg is not None:
        if isinstance(msg, NoteOn):
            if msg.note in midi_notes:
                play_note(msg.note, msg.velocity > 0)

        if isinstance(msg, NoteOff):
            if msg.note in midi_notes:
                play_note(msg.note, False)

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
