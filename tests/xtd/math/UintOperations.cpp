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
    size_t i = 1;
    for(; i < 64; ++i) {
        auto n = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_u256;
        shiftLeft(n, i);

        EXPECT_EQ(n[0], std::numeric_limits<uint64_t>::max() << i) << "Mismatch at shift " << i;
        EXPECT_EQ(n[1], std::numeric_limits<uint64_t>::max()) << "Mismatch at shift " << i;
        EXPECT_EQ(n[2], std::numeric_limits<uint64_t>::max() >> (64 - i)) << "Mismatch at shift " << i;
        EXPECT_EQ(n[3], 0) << "Mismatch at shift " << i;
    }
    {
        auto n = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_u256;
        shiftLeft(n, i);

        EXPECT_EQ(n[0], 0) << "Mismatch at shift " << i;
        EXPECT_EQ(n[1], std::numeric_limits<uint64_t>::max()) << "Mismatch at shift " << i;
        EXPECT_EQ(n[2], std::numeric_limits<uint64_t>::max()) << "Mismatch at shift " << i;
        EXPECT_EQ(n[3], 0) << "Mismatch at shift " << i;
        ++i;
    }
    for(; i < 128; ++i) {
        auto n = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_u256;
        shiftLeft(n, i);

        EXPECT_EQ(n[0], 0) << "Mismatch at shift " << i;
        EXPECT_EQ(n[1], std::numeric_limits<uint64_t>::max() << (i - 64)) << "Mismatch at shift " << i;
        EXPECT_EQ(n[2], std::numeric_limits<uint64_t>::max()) << "Mismatch at shift " << i;
        EXPECT_EQ(n[3], std::numeric_limits<uint64_t>::max() >> (128 - i)) << "Mismatch at shift " << i;
    }
}

TEST_F(UintTest, BasicFunctionality_ShiftRight) {
    size_t i = 1;
    for(; i < 64; ++i) {
        auto n = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000000000000000_u256;
        shiftRight(n, i);

        EXPECT_EQ(n[0], 0) << "Mismatch at shift " << i;
        EXPECT_EQ(n[1], std::numeric_limits<uint64_t>::max() << (64 - i)) << "Mismatch at shift " << i;
        EXPECT_EQ(n[2], std::numeric_limits<uint64_t>::max()) << "Mismatch at shift " << i;
        EXPECT_EQ(n[3], std::numeric_limits<uint64_t>::max() >> i) << "Mismatch at shift " << i;
    }
    {
        auto n = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000000000000000_u256;
        shiftRight(n, i);

        EXPECT_EQ(n[0], 0) << "Mismatch at shift " << i;
        EXPECT_EQ(n[1], std::numeric_limits<uint64_t>::max()) << "Mismatch at shift " << i;
        EXPECT_EQ(n[2], std::numeric_limits<uint64_t>::max()) << "Mismatch at shift " << i;
        EXPECT_EQ(n[3], 0) << "Mismatch at shift " << i;
        ++i;
    }
    for(; i < 128; ++i) {
        auto n = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000000000000000_u256;
        shiftRight(n, i);

        EXPECT_EQ(n[0], std::numeric_limits<uint64_t>::max() << (128 - i)) << "Mismatch at shift " << i;
        EXPECT_EQ(n[1], std::numeric_limits<uint64_t>::max()) << "Mismatch at shift " << i;
        EXPECT_EQ(n[2], std::numeric_limits<uint64_t>::max() >> (i - 64)) << "Mismatch at shift " << i;
        EXPECT_EQ(n[3], 0) << "Mismatch at shift " << i;
    }
}

TEST_F(UintTest, BasicFunctionality_Add) {
    auto l = 0x0000000000000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_u;
    auto r = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000000000000_u;
    auto res = add(l , r).first;
    EXPECT_EQ(res, 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF_u);
}

} // namespace