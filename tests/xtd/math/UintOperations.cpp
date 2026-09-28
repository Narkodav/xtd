#include <gtest/gtest.h>
#include "xtd/math/Utilities.h"
#include "xtd/math/Uint.h"

#include "../Helpers.h"

#include <algorithm>
#include <numeric>

namespace {

using namespace xtd::math;

class UintTest : public ::testing::Test {};

// ---------------------------------------------------------------------
// Uint
// ---------------------------------------------------------------------

TEST_F(UintTest, BasicFunctionality_ShiftLeft) {
    UintImpl<8, uint8_t> n1 = std::numeric_limits<uint64_t>::max();
    uint64_t n2 = std::numeric_limits<uint64_t>::max();

    for(size_t i = 0; i < 64; ++i) {
        n1 = std::numeric_limits<uint64_t>::max();
        n2 = std::numeric_limits<uint64_t>::max();
        shiftLeft(n1, i);
        n2 <<= i;

        for(size_t j = 0; j < n1.size(); ++j) {
            EXPECT_EQ(n1[j], (n2 & (static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()) << (8 * j))) >> (8 * j))
                << "Mismatch at shift " << i;
        }
    }
}

TEST_F(UintTest, BasicFunctionality_ShiftRight) {
    UintImpl<8, uint8_t> n1 = std::numeric_limits<uint64_t>::max();
    uint64_t n2 = std::numeric_limits<uint64_t>::max();

    for(size_t i = 0; i < 64; ++i) {
        n1 = std::numeric_limits<uint64_t>::max();
        n2 = std::numeric_limits<uint64_t>::max();
        shiftRight(n1, i);
        n2 >>= i;

        for(size_t j = 0; j < n1.size(); ++j) {
            EXPECT_EQ(n1[j], (n2 & (static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()) << (8 * j))) >> (8 * j))
                << "Mismatch at shift " << i;
        }
    }
}

TEST_F(UintTest, BasicFunctionality_Add) {
    UintImpl<4, uint64_t> l = std::numeric_limits<uint64_t>::max();
    UintImpl<4, uint64_t> r = std::numeric_limits<uint64_t>::max();

    for(size_t i = 0; i < 64; ++i) {
        n1 = std::numeric_limits<uint64_t>::max();
        n2 = std::numeric_limits<uint64_t>::max();
        shiftRight(n1, i);
        n2 >>= i;

        for(size_t j = 0; j < n1.size(); ++j) {
            EXPECT_EQ(n1[j], (n2 & (static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()) << (8 * j))) >> (8 * j))
                << "Mismatch at shift " << i;
        }
    }
}

} // namespace