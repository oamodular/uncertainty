#define SAMPLE_RATE 40000
#define SINDEX_MAX 0x7FFFFFFF
#define MS_DELTA (SAMPLE_RATE/1000)
#define SAMPLE_DELTA (SINDEX_MAX/SAMPLE_RATE)

class Phasor {
public:
  uint32_t phase;
  uint32_t delta;
  bool reset;
  Phasor() {
    this->phase = 0;
    this->delta = SAMPLE_DELTA;
    this->reset = true;
  }
  Phasor(int freq) {
    this->phase = 0;
    this->SetFreq(freq);
  }
  void SetFreq(int freq) {
    this->delta = SAMPLE_DELTA*freq;
  }
  void SetDuration(int ms) {
    this->delta = (SAMPLE_DELTA*1000)/ms;
  }
  bool IsAtEnd() {
    return phase >= SINDEX_MAX;
  }
  int Process() {
    int out = phase;
    phase += delta;
    if(phase >= SINDEX_MAX) {
      if(reset) {
        phase -= SINDEX_MAX;
      } else {
        phase = SINDEX_MAX;
      }
    }
    return out >> (31-7); // output 0 - 127 inclusive
  }
};

class Env {
public:
  Phasor attack;
  Phasor decay;
  Env() {
    attack.SetDuration(5);
    attack.reset = false;
    decay.SetDuration(150);
    decay.reset = false;
  }
  void Reset() {
    attack.phase = 0; decay.phase = 0;
  }
  int Process() {
    int attackOut = 127;
    int decayOut = 127;
    if(attack.IsAtEnd()) {
      decayOut = 127 - decay.Process();
    } else {
      attackOut = attack.Process();
    }
    return min(attackOut, decayOut);
  }
};

class TrigGen {
public:
  int samplesSinceFired;
  int width;
  int amp;
  TrigGen(int widthInSamples = 160, int amplitude = 127) {
    samplesSinceFired =  width + 1;
    width = widthInSamples;
    amp = amplitude;
  }
  void Reset() { samplesSinceFired = -1; }
  int Process() {
    samplesSinceFired++;    
    return (samplesSinceFired < width) ? amp : 0;
  }
};
