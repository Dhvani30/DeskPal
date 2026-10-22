#include "../DeskPal/comfort_learner.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static unsigned int rng = 0xC0FFEEu;
static unsigned int randomU32() { rng = rng * 1664525u + 1013904223u; return rng; }
static float randomUnit() { return (randomU32() >> 8) / 16777216.0f; }

static bool finiteDifferenceCheck() {
  ComfortLearner learner(1.2f, 2.0f, 12, 30, 18, 38, 2);
  learner.init(18, 26);
  const float x = 24.1f, lo = 19.3f, hi = 25.4f, epsilon = 0.0001f;
  float gradLo, gradHi;
  learner.gradientFor(x, ComfortLearner::HOT, lo, hi, gradLo, gradHi);
  const double numericLo = (learner.lossFor(x, ComfortLearner::HOT, lo + epsilon, hi) -
                            learner.lossFor(x, ComfortLearner::HOT, lo - epsilon, hi)) / (2 * epsilon);
  const double numericHi = (learner.lossFor(x, ComfortLearner::HOT, lo, hi + epsilon) -
                            learner.lossFor(x, ComfortLearner::HOT, lo, hi - epsilon)) / (2 * epsilon);
  printf("gradient check: lo analytic %.6f numeric %.6f; hi analytic %.6f numeric %.6f\n",
         gradLo, numericLo, gradHi, numericHi);
  return fabs(gradLo - numericLo) < 0.002 && fabs(gradHi - numericHi) < 0.002;
}

static ComfortLearner::Label hiddenVote(float x, float lo, float hi) {
  if (x < lo) return ComfortLearner::COLD;
  if (x > hi) return ComfortLearner::HOT;
  return ComfortLearner::PERFECT;
}

static ComfortLearner::Label noisyVote(float x, float lo, float hi) {
  ComfortLearner::Label label = hiddenVote(x, lo, hi);
  if (randomUnit() < 0.10f) {
    unsigned int alternative = randomU32() % 2;
    if (label == ComfortLearner::COLD) return alternative ? ComfortLearner::HOT : ComfortLearner::PERFECT;
    if (label == ComfortLearner::HOT) return alternative ? ComfortLearner::COLD : ComfortLearner::PERFECT;
    return alternative ? ComfortLearner::HOT : ComfortLearner::COLD;
  }
  return label;
}

static bool inBounds(const ComfortLearner& learner, float loMin, float loMax,
                     float hiMin, float hiMax, float gap) {
  return isfinite(learner.lo()) && isfinite(learner.hi()) &&
         learner.lo() >= loMin && learner.lo() <= loMax &&
         learner.hi() >= hiMin && learner.hi() <= hiMax && learner.hi() - learner.lo() >= gap;
}

int main() {
  if (!finiteDifferenceCheck()) { fprintf(stderr, "finite-difference gradient check failed\n"); return 1; }

  ComfortLearner temperature(1.2f, 2.0f, 12, 30, 18, 38, 2);
  // Temperate climate defaults from getClimateParams(): 18-26 C.
  temperature.init(18, 26);
  const float hiddenLo = 22.0f, hiddenHi = 26.5f;
  const unsigned int checkpoints[] = {5, 10, 20, 40};
  printf("votes | learned C | hidden C | endpoint error\n");
  unsigned int checkpoint = 0;
  for (unsigned int i = 1; i <= 40; ++i) {
    // Repeatedly drift from 18 C to 32 C and back, covering both edges.
    const unsigned int position = (i - 1) % 16;
    const unsigned int distance = position <= 8 ? position : 16 - position;
    const float x = 18.0f + 14.0f * distance / 8.0f;
    temperature.vote(x, noisyVote(x, hiddenLo, hiddenHi));
    if (checkpoint < 4 && i == checkpoints[checkpoint]) {
      const float error = fabsf(temperature.lo() - hiddenLo) + fabsf(temperature.hi() - hiddenHi);
      printf("%5u | %5.2f-%5.2f | %5.1f-%5.1f | %.2f C\n", i, temperature.lo(), temperature.hi(), hiddenLo, hiddenHi, error);
      ++checkpoint;
    }
  }
  const float learnedError = fabsf(temperature.lo() - hiddenLo) + fabsf(temperature.hi() - hiddenHi);
  const float defaultError = fabsf(18.0f - hiddenLo) + fabsf(26.0f - hiddenHi);
  printf("default band error: %.2f C; learned band error: %.2f C\n", defaultError, learnedError);
  if (fabsf(temperature.lo() - hiddenLo) > 1.0f || fabsf(temperature.hi() - hiddenHi) > 1.0f || learnedError >= defaultError) {
    fprintf(stderr, "40-vote temperature learning requirement failed\n"); return 2;
  }

  ComfortLearner humidity(0.15f, 12.0f, 15, 60, 50, 95, 15);
  humidity.init(30, 85);
  for (unsigned int i = 0; i < 40; ++i) {
    const float x = 25.0f + (float)((i * 29u) % 701u) / 10.0f;
    humidity.vote(x, noisyVote(x, 35.0f, 65.0f));
  }
  printf("humidity after 40 votes: %.2f-%.2f%% (hidden 35-65%%)\n", humidity.lo(), humidity.hi());

  ComfortLearner same(1.2f, 2.0f, 12, 30, 18, 38, 2);
  same.init(18, 26);
  for (int i = 0; i < 40; ++i) same.vote(1000.0f, ComfortLearner::HOT);
  if (!inBounds(same, 12, 30, 18, 38, 2)) { fprintf(stderr, "same-label bounds failed\n"); return 3; }
  ComfortLearner contradictory(1.2f, 2.0f, 12, 30, 18, 38, 2);
  contradictory.init(18, 26);
  for (int i = 0; i < 100; ++i) contradictory.vote(24.0f, i % 2 ? ComfortLearner::HOT : ComfortLearner::COLD);
  if (!inBounds(contradictory, 12, 30, 18, 38, 2)) { fprintf(stderr, "contradictory-vote bounds failed\n"); return 4; }
  const unsigned int beforeNanVotes = temperature.votes();
  const float beforeNanLo = temperature.lo(), beforeNanHi = temperature.hi();
  if (temperature.vote(NAN, ComfortLearner::HOT) || temperature.votes() != beforeNanVotes ||
      temperature.lo() != beforeNanLo || temperature.hi() != beforeNanHi) {
    fprintf(stderr, "NaN input validation failed\n"); return 5;
  }

  unsigned char state[ComfortLearner::SERIALIZED_SIZE];
  ComfortLearner restored(1.2f, 2.0f, 12, 30, 18, 38, 2);
  restored.init(18, 26);
  if (!temperature.serialize(state, sizeof(state)) || !restored.deserialize(state, sizeof(state)) ||
      restored.votes() != temperature.votes() || fabsf(restored.lo() - temperature.lo()) > 1e-6f ||
      fabsf(restored.hi() - temperature.hi()) > 1e-6f) {
    fprintf(stderr, "serialization roundtrip failed\n"); return 6;
  }
  state[1] = 0; state[2] = 0; state[3] = 0; state[4] = 0x7f;
  if (restored.deserialize(state, sizeof(state))) { fprintf(stderr, "serialized infinity validation failed\n"); return 7; }
  if (!temperature.undoLast() || temperature.votes() != beforeNanVotes - 1 || temperature.undoLast()) {
    fprintf(stderr, "single-state undo failed\n"); return 8;
  }
  printf("robustness: same-label, contradictory, NaN, serialization, undo passed\n");
  return 0;
}
