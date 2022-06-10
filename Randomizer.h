#ifndef SIK_ZAD2_RANDOMIZER_H
#define SIK_ZAD2_RANDOMIZER_H

#include <cstdint>

#include "MessageUtils.h"

class Randomizer {
 private:
  uint64_t last_val{};

 public:
  explicit Randomizer(uint32_t seed) : last_val(seed){};
  uint32_t get_next_val() {
    last_val = (uint32_t)((last_val * 48271) % 2147483647);

    return (uint32_t)last_val;
  }

  Position get_next_position(uint16_t size_x, uint16_t size_y) {
    uint16_t pos_x, pos_y;

    pos_x = (uint16_t)(get_next_val() % (uint32_t)size_x);
    pos_y = (uint16_t)(get_next_val() % (uint32_t)size_y);

    return Position(pos_x, pos_y);
  }
};

#endif  // SIK_ZAD2_RANDOMIZER_H
