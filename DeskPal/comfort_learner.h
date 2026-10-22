#ifndef DESKPAL_COMFORT_LEARNER_H
#define DESKPAL_COMFORT_LEARNER_H

#include <math.h>
#include <string.h>

// Small, fixed-size online estimate of a user's preferred comfort band.
class ComfortLearner {
 public:
  enum Label { HOT = 0, COLD = 1, PERFECT = 2 };
  enum { SERIALIZED_SIZE = 26 };

  ComfortLearner(float slope, float priorStd, float loMin, float loMax,
                 float hiMin, float hiMax, float minGap)
      : k_(slope), priorStd_(priorStd), loMin_(loMin), loMax_(loMax),
        hiMin_(hiMin), hiMax_(hiMax), minGap_(minGap), priorLo_(loMin),
        priorHi_(hiMax), lo_(loMin), hi_(hiMax), votes_(0), hasPrevious_(false),
        previousLo_(loMin), previousHi_(hiMax), previousVotes_(0) {}

  void init(float lo0, float hi0) {
    priorLo_ = lo0;
    priorHi_ = hi0;
    lo_ = lo0;
    hi_ = hi0;
    project(lo_, hi_);
    priorLo_ = lo_;
    priorHi_ = hi_;
    votes_ = 0;
    hasPrevious_ = false;
    previousLo_ = lo_;
    previousHi_ = hi_;
    previousVotes_ = 0;
  }

  bool vote(float x, Label label) {
    if (!isfinite((double)x) || label < HOT || label > PERFECT) return false;
    previousLo_ = lo_;
    previousHi_ = hi_;
    previousVotes_ = votes_;
    hasPrevious_ = true;
    float gradLo, gradHi;
    gradientFor(x, label, lo_, hi_, gradLo, gradHi);
    const float eta = fmaxf(0.15f, 1.0f / (1.0f + (float)votes_ / 10.0f));
    lo_ -= eta * gradLo;
    hi_ -= eta * gradHi;
    project(lo_, hi_);
    if (votes_ != 0xFFFFFFFFU) ++votes_;
    return true;
  }

  float lo() const { return lo_; }
  float hi() const { return hi_; }
  unsigned int votes() const { return votes_; }
  float confidence() const { return fminf(1.0f, (float)votes_ / 20.0f); }
  bool undoLast() {
    if (!hasPrevious_) return false;
    lo_ = previousLo_;
    hi_ = previousHi_;
    votes_ = previousVotes_;
    hasPrevious_ = false;
    return true;
  }
  void reset() { init(priorLo_, priorHi_); }

  // Stable negative log-likelihood plus the Gaussian MAP prior.
  double lossFor(float x, Label label, float lo, float hi) const {
    const double hotTarget = label == HOT ? 1.0 : 0.0;
    const double coldTarget = label == COLD ? 1.0 : 0.0;
    const double hotZ = (double)k_ * ((double)x - hi);
    const double coldZ = (double)k_ * ((double)lo - x);
    const double sigma2 = (double)priorStd_ * priorStd_;
    const double priorWeight = 1.0 / ((double)votes_ + 1.0);
    return softplus(hotZ) - hotTarget * hotZ + softplus(coldZ) - coldTarget * coldZ +
           priorWeight * 0.5 * ((double)hi - priorHi_) * ((double)hi - priorHi_) / sigma2 +
           priorWeight * 0.5 * ((double)lo - priorLo_) * ((double)lo - priorLo_) / sigma2;
  }

  void gradientFor(float x, Label label, float lo, float hi,
                   float& gradLo, float& gradHi) const {
    const float hotTarget = label == HOT ? 1.0f : 0.0f;
    const float coldTarget = label == COLD ? 1.0f : 0.0f;
    const float pHot = sigmoid(k_ * (x - hi));
    const float pCold = sigmoid(k_ * (lo - x));
    const float sigma2 = priorStd_ * priorStd_;
    // In online MAP updates, share the single prior across accumulated votes.
    const float priorWeight = 1.0f / ((float)votes_ + 1.0f);
    gradHi = k_ * (hotTarget - pHot) + priorWeight * (hi - priorHi_) / sigma2;
    gradLo = k_ * (pCold - coldTarget) + priorWeight * (lo - priorLo_) / sigma2;
  }

  bool serialize(unsigned char* out, size_t capacity) const {
    if (!out || capacity < SERIALIZED_SIZE) return false;
    size_t p = 0;
    out[p++] = 1;
    memcpy(out + p, &lo_, sizeof(float)); p += sizeof(float);
    memcpy(out + p, &hi_, sizeof(float)); p += sizeof(float);
    writeU32(out + p, votes_); p += 4;
    out[p++] = hasPrevious_ ? 1 : 0;
    memcpy(out + p, &previousLo_, sizeof(float)); p += sizeof(float);
    memcpy(out + p, &previousHi_, sizeof(float)); p += sizeof(float);
    writeU32(out + p, previousVotes_);
    return true;
  }

  bool deserialize(const unsigned char* in, size_t length) {
    if (!in || length != SERIALIZED_SIZE || in[0] != 1 || in[13] > 1) return false;
    size_t p = 1;
    float lo, hi, previousLo, previousHi;
    memcpy(&lo, in + p, sizeof(float)); p += sizeof(float);
    memcpy(&hi, in + p, sizeof(float)); p += sizeof(float);
    const unsigned int votes = readU32(in + p); p += 4;
    const bool hasPrevious = in[p++] != 0;
    memcpy(&previousLo, in + p, sizeof(float)); p += sizeof(float);
    memcpy(&previousHi, in + p, sizeof(float)); p += sizeof(float);
    const unsigned int previousVotes = readU32(in + p);
    if (!validBand(lo, hi) || !validBand(previousLo, previousHi) ||
        (hasPrevious && previousVotes > votes)) return false;
    lo_ = lo; hi_ = hi; votes_ = votes;
    hasPrevious_ = hasPrevious;
    previousLo_ = previousLo; previousHi_ = previousHi;
    previousVotes_ = previousVotes;
    return true;
  }

 private:
  float k_, priorStd_, loMin_, loMax_, hiMin_, hiMax_, minGap_;
  float priorLo_, priorHi_, lo_, hi_;
  unsigned int votes_;
  bool hasPrevious_;
  float previousLo_, previousHi_;
  unsigned int previousVotes_;

  static float sigmoid(float z) {
    if (z >= 0.0f) return 1.0f / (1.0f + expf(-z));
    const float ez = expf(z);
    return ez / (1.0f + ez);
  }
  static double softplus(double z) {
    return z > 0.0 ? z + log1p(exp(-z)) : log1p(exp(z));
  }
  bool validBand(float lo, float hi) const {
    return isfinite((double)lo) && isfinite((double)hi) &&
           lo >= loMin_ && lo <= loMax_ && hi >= hiMin_ && hi <= hiMax_ &&
           hi - lo >= minGap_;
  }
  void project(float& lo, float& hi) const {
    lo = fminf(loMax_, fmaxf(loMin_, lo));
    hi = fminf(hiMax_, fmaxf(hiMin_, hi));
    if (hi - lo < minGap_) {
      const float half = minGap_ * 0.5f;
      const float center = (lo + hi) * 0.5f;
      lo = center - half;
      hi = center + half;
      if (lo < loMin_) { hi += loMin_ - lo; lo = loMin_; }
      if (hi > hiMax_) { lo -= hi - hiMax_; hi = hiMax_; }
      lo = fminf(loMax_, fmaxf(loMin_, lo));
      hi = fminf(hiMax_, fmaxf(hiMin_, hi));
    }
  }
  static void writeU32(unsigned char* out, unsigned int value) {
    out[0] = (unsigned char)value; out[1] = (unsigned char)(value >> 8);
    out[2] = (unsigned char)(value >> 16); out[3] = (unsigned char)(value >> 24);
  }
  static unsigned int readU32(const unsigned char* in) {
    return (unsigned int)in[0] | ((unsigned int)in[1] << 8) |
           ((unsigned int)in[2] << 16) | ((unsigned int)in[3] << 24);
  }
};

#endif
