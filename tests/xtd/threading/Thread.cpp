#include <gtest/gtest.h>
#include "xtd/threading/Thread.h"

#include "../Helpers.h"
#include "xtd/threading/Latch.h"
#include "xtd/time/Duration.h"

#include <algorithm>
#include <numeric>

namespace {

using xtd::threading::Thread;

class ThreadTest : public ::testing::Test {};

// ---------------------------------------------------------------------
// Thread
// ---------------------------------------------------------------------

    TEST_F(ThreadTest, DefaultConstruction_IsNotJoinable) {
        xtd::threading::Thread thread;
        EXPECT_EQ(thread.joinable(), false);
    }

    TEST_F(ThreadTest, DefaultConstruction_CreatesEmptyProcess) {
        xtd::threading::Thread thread;
        thread.create([](){});
        EXPECT_EQ(thread.joinable(), true);
    }

    TEST_F(ThreadTest, DefaultConstruction_JoinsCorrectly) {
        xtd::threading::Thread thread;
        thread.create([](){});
        thread.join();
        EXPECT_EQ(thread.joinable(), false);
    }

    TEST_F(ThreadTest, DefaultConstruction_JoinWaitsForCompletion) {
        xtd::threading::Thread thread;
        xtd::time::Second<int> i;
        xtd::threading::Latch latch;
        thread.create([&latch](){
            xtd::threading::Thread::sleep(xtd::time::Second<int>(1));
            latch.release();
        });
        thread.join();
        EXPECT_EQ(thread.joinable(), false);
        latch.acquire();
        EXPECT_EQ(latch.done(), true);
    }
} // namespace