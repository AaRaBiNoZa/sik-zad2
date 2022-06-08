#ifndef SIK_ZAD2_RANDOMIZER_H
#define SIK_ZAD2_RANDOMIZER_H

#include <cstdint>
class Randomizer {
 private:
  uint64_t last_val{};
 public:
  explicit Randomizer(uint32_t seed) : last_val(seed) {};
  uint32_t getNextVal() {
    last_val = (last_val * 48271) % 2147483647;
    return (uint32_t) last_val;
  }
};

#endif  // SIK_ZAD2_RANDOMIZER_H
