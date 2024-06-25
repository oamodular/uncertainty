from oam import Uncertainty
import random

unc = Uncertainty('pwm')

class Walk:

  def __init__ (self, range, mode='clip'):
    self.range = range
    self.mode = mode
    self.pos = 0

  def step(self):
    if (random.randrange(0, 2)):
        if (self.pos==self.range):
            if (self.mode=='clip'):
                return self.pos
        else:
            self.pos = self.pos+1
            return self.pos
    else:
        if (self.pos==0):
            return self.pos
        else:
            self.pos = self.pos-1
            return self.pos

walkLengths = [2,4,7,11,15,31,63,127]
walks = []

for leng in walkLengths:
    walks.append(Walk(leng))

fiveV = 65535

while True:

    if unc.isRisingEdge():
        for i, w in enumerate(walks):
            unc.outs[i].duty_cycle = int(w.step() * (fiveV/walkLengths[i]))
