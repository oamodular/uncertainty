prob = [99,91,67,50,33,13,5,1]

from oam import Uncertainty
import random

unc = Uncertainty()

while True:

    if unc.isGateOff():
        unc.lightsOut()
    elif unc.isRisingEdge():
        for i in range(8):
            flip = random.randrange(100) < prob[i]
            unc.outs[i].value = flip
