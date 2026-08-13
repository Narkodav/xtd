#include <gtest/gtest.h>
#include "xtd/memory/UniquePtr.h"

#include "../Helpers.h"

#include <algorithm>
#include <numeric>

namespace {

using xtd::memory::UniquePtr;

// A fixture that resets the tracker's static counters before every test,
// so tests remain independent of each other regardless of run order.
class UniquePtrScalarTest : public ::testing::Test {
protected:
    void SetUp() override { LifetimeTracker::resetCounts(); }
};

// ---------------------------------------------------------------------
// Scalar UniquePtr<T>
// ---------------------------------------------------------------------

TEST_F(UniquePtrScalarTest, DefaultConstruction_IsNull) {
    UniquePtr<LifetimeTracker> p;
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(LifetimeTracker::constructedCount, 0);
}

TEST_F(UniquePtrScalarTest, Make_ConstructsExactlyOneValue) {
    auto p = UniquePtr<LifetimeTracker>::make(42);
    EXPECT_EQ(LifetimeTracker::constructedCount, 1);
    EXPECT_NE(p, nullptr);
}

TEST_F(UniquePtrScalarTest, Dereference_ReturnsConstructedValue) {
    auto p = UniquePtr<LifetimeTracker>::make(42);
    EXPECT_EQ((*p).value, 42);
    EXPECT_EQ(p->value, 42);
}

TEST_F(UniquePtrScalarTest, Dereference_AllowsMutation) {
    auto p = UniquePtr<LifetimeTracker>::make(1);
    p->value = 99;
    EXPECT_EQ(p->value, 99);
}

TEST_F(UniquePtrScalarTest, Destructor_DestroysValueExactlyOnce) {
    {
        auto p = UniquePtr<LifetimeTracker>::make(1);
        EXPECT_EQ(LifetimeTracker::destroyedCount, 0);
    }
    EXPECT_EQ(LifetimeTracker::destroyedCount, 1);
}

TEST_F(UniquePtrScalarTest, Reset_DestroysValueAndNullifiesPointer) {
    auto p = UniquePtr<LifetimeTracker>::make(1);
    p.reset();
    EXPECT_EQ(LifetimeTracker::destroyedCount, 1);
    EXPECT_EQ(p, nullptr);
}

TEST_F(UniquePtrScalarTest, Reset_OnAlreadyNullPointer_IsNoOp) {
    UniquePtr<LifetimeTracker> p;
    EXPECT_NO_THROW(p.reset());
    EXPECT_EQ(LifetimeTracker::destroyedCount, 0);
}

TEST_F(UniquePtrScalarTest, MoveConstruction_TransfersOwnership) {
    auto original = UniquePtr<LifetimeTracker>::make(7);
    auto* rawAddress = original.data();

    UniquePtr<LifetimeTracker> moved(std::move(original));

    EXPECT_EQ(moved.data(), rawAddress);
    EXPECT_EQ(moved->value, 7);
}

TEST_F(UniquePtrScalarTest, MoveConstruction_SourceBecomesNull) {
    auto original = UniquePtr<LifetimeTracker>::make(7);
    UniquePtr<LifetimeTracker> moved(std::move(original));

    EXPECT_EQ(original, nullptr); // NOLINT(bugprone-use-after-move) - intentional check
}

TEST_F(UniquePtrScalarTest, MoveConstruction_DoesNotConstructOrDestroyValue) {
    auto original = UniquePtr<LifetimeTracker>::make(7);
    LifetimeTracker::resetCounts();

    UniquePtr<LifetimeTracker> moved(std::move(original));

    EXPECT_EQ(LifetimeTracker::constructedCount, 0);
    EXPECT_EQ(LifetimeTracker::destroyedCount, 0);
}

TEST_F(UniquePtrScalarTest, MoveAssignment_DestroysPreviouslyOwnedValue) {
    auto a = UniquePtr<LifetimeTracker>::make(1);
    auto b = UniquePtr<LifetimeTracker>::make(2);

    a = std::move(b);

    // The value originally owned by `a` (value 1) should have been destroyed.
    EXPECT_EQ(LifetimeTracker::destroyedCount, 1);
    EXPECT_EQ(a->value, 2);
}

TEST_F(UniquePtrScalarTest, MoveAssignment_SourceBecomesNull) {
    auto a = UniquePtr<LifetimeTracker>::make(1);
    auto b = UniquePtr<LifetimeTracker>::make(2);

    a = std::move(b);

    EXPECT_EQ(b, nullptr); // NOLINT(bugprone-use-after-move)
}

TEST_F(UniquePtrScalarTest, MoveAssignment_SelfAssignmentIsSafe) {
    auto a = UniquePtr<LifetimeTracker>::make(1);
    auto* rawAddress = a.data();

    a = std::move(a);

    EXPECT_EQ(a.data(), rawAddress);
    EXPECT_EQ(LifetimeTracker::destroyedCount, 0);
}

TEST_F(UniquePtrScalarTest, Release_ReturnsOwnershipWithoutDestroyingValue) {
    auto p = UniquePtr<LifetimeTracker>::make(5);
    auto* rawAddress = p.data();

    auto parts = p.release();

    EXPECT_EQ(parts.data, rawAddress);
    EXPECT_EQ(LifetimeTracker::destroyedCount, 0);
    EXPECT_EQ(p, nullptr);

    parts.cleanup(); // caller now owns cleanup
    EXPECT_EQ(LifetimeTracker::destroyedCount, 1);
}

TEST_F(UniquePtrScalarTest, EqualityWithNullptr) {
    UniquePtr<LifetimeTracker> empty;
    auto full = UniquePtr<LifetimeTracker>::make(1);

    EXPECT_TRUE(empty == nullptr);
    EXPECT_FALSE(full == nullptr);
}

TEST_F(UniquePtrScalarTest, EqualityBetweenTwoUniquePtrs_ComparesAddress) {
    auto a = UniquePtr<LifetimeTracker>::make(1);
    UniquePtr<LifetimeTracker> b; // null

    EXPECT_FALSE(a == b);
}

TEST_F(UniquePtrScalarTest, ComparisonWithRawPointer_MatchesUnderlyingAddress) {
    auto p = UniquePtr<LifetimeTracker>::make(1);
    LifetimeTracker* raw = p.data();

    EXPECT_TRUE(p == raw);
}

// ---------------------------------------------------------------------
// Array UniquePtr<T[]>
// ---------------------------------------------------------------------

class UniquePtrArrayTest : public ::testing::Test {
protected:
    void SetUp() override { LifetimeTracker::resetCounts(); }
};

TEST_F(UniquePtrArrayTest, DefaultConstruction_IsNull) {
    UniquePtr<int[]> p;
    EXPECT_EQ(p, nullptr);
}

TEST_F(UniquePtrArrayTest, Make_FromInitializerList_HasCorrectSizeAndValues) {
    auto p = UniquePtr<int[]>::make({1, 2, 3, 4});

    ASSERT_EQ(p.size(), 4u);
    EXPECT_EQ(p[0], 1);
    EXPECT_EQ(p[1], 2);
    EXPECT_EQ(p[2], 3);
    EXPECT_EQ(p[3], 4);
}

TEST_F(UniquePtrArrayTest, Make_WithSizeOnly_DefaultConstructsElements) {
    auto p = UniquePtr<int[]>::make(size_t{5});

    ASSERT_EQ(p.size(), 5u);
    for (size_t i = 0; i < p.size(); ++i) {
        EXPECT_EQ(p[i], 0);
    }
}

TEST_F(UniquePtrArrayTest, Make_WithSizeAndFillValue_InitializesAllElements) {
    auto p = UniquePtr<int[]>::make(size_t{4}, 7);

    ASSERT_EQ(p.size(), 4u);
    for (size_t i = 0; i < p.size(); ++i) {
        EXPECT_EQ(p[i], 7);
    }
}

TEST_F(UniquePtrArrayTest, IndexOperator_AllowsMutation) {
    auto p = UniquePtr<int[]>::make({1, 2, 3});
    p[1] = 99;
    EXPECT_EQ(p[1], 99);
}

TEST_F(UniquePtrArrayTest, BeginEnd_SupportRangeBasedFor) {
    auto p = UniquePtr<int[]>::make({10, 20, 30});

    int sum = 0;
    for (int v : p) sum += v;

    EXPECT_EQ(sum, 60);
}

TEST_F(UniquePtrArrayTest, BeginEnd_WorkWithStdAlgorithms) {
    auto p = UniquePtr<int[]>::make({3, 1, 2});
    EXPECT_TRUE(std::is_permutation(p.begin(), p.end(),
                                     std::initializer_list<int>{1, 2, 3}.begin()));
}

TEST_F(UniquePtrArrayTest, Size_MatchesNumberOfElementsRequested) {
    auto p = UniquePtr<int[]>::make(size_t{10}, 0);
    EXPECT_EQ(p.size(), 10u);
}

TEST_F(UniquePtrArrayTest, Destructor_DestroysEveryElementExactlyOnce) {
    {
        auto p = UniquePtr<LifetimeTracker[]>::make(size_t{5}, LifetimeTracker(0, false));
        EXPECT_EQ(LifetimeTracker::destroyedCount, 0);
    }
    EXPECT_EQ(LifetimeTracker::destroyedCount, 5);
}

TEST_F(UniquePtrArrayTest, MoveConstruction_TransfersOwnership) {
    auto original = UniquePtr<int[]>::make({1, 2, 3});
    auto* rawAddress = original.data();

    UniquePtr<int[]> moved(std::move(original));

    EXPECT_EQ(moved.data(), rawAddress);
    EXPECT_EQ(moved.size(), 3u);
}

TEST_F(UniquePtrArrayTest, MoveConstruction_SourceBecomesNull) {
    auto original = UniquePtr<int[]>::make({1, 2, 3});
    UniquePtr<int[]> moved(std::move(original));

    EXPECT_EQ(original, nullptr); // NOLINT(bugprone-use-after-move)
}

TEST_F(UniquePtrArrayTest, MoveAssignment_DestroysPreviouslyOwnedElements) {
    auto a = UniquePtr<LifetimeTracker[]>::make(size_t{2}, LifetimeTracker(0, false));
    auto b = UniquePtr<LifetimeTracker[]>::make(size_t{3}, LifetimeTracker(0, false));
    LifetimeTracker::resetCounts();

    a = std::move(b);

    EXPECT_EQ(LifetimeTracker::destroyedCount, 2); // a's original 2 elements
    EXPECT_EQ(a.size(), 3u);
}

TEST_F(UniquePtrArrayTest, Release_ReturnsRangeWithoutDestroyingElements) {
    auto p = UniquePtr<LifetimeTracker[]>::make(size_t{3}, LifetimeTracker(0, false));
    auto parts = p.release();

    EXPECT_EQ(LifetimeTracker::destroyedCount, 0);
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(parts.end - parts.begin, 3);

    parts.cleanup();
    EXPECT_EQ(LifetimeTracker::destroyedCount, 3);
}

// ---------------------------------------------------------------------
// Parameterized tests: exercise Make(size, value) across a range of
// sizes instead of writing one near-identical TEST per size.
// ---------------------------------------------------------------------

class UniquePtrArraySizeTest : public ::testing::TestWithParam<size_t> {};

TEST_P(UniquePtrArraySizeTest, MakeWithSize_ProducesRequestedElementCount) {
    const size_t requestedSize = GetParam();
    auto p = UniquePtr<int[]>::make(requestedSize, 3);

    EXPECT_EQ(p.size(), requestedSize);
    for (size_t i = 0; i < p.size(); ++i) {
        EXPECT_EQ(p[i], 3);
    }
}

INSTANTIATE_TEST_SUITE_P(
    VariousSizes,
    UniquePtrArraySizeTest,
    ::testing::Values(size_t{0}, size_t{1}, size_t{2}, size_t{16}, size_t{100})
);

} // namespace