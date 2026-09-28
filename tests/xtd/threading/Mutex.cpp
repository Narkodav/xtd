#include <gtest/gtest.h>
#include "xtd/threading/Mutex.h"
#include "xtd/threading/SharedMutex.h"
#include "xtd/threading/Thread.h"
#include "xtd/containers/Vector.h"
#include "xtd/memory/UniquePtr.h"

#include "../Helpers.h"

#include <algorithm>
#include <numeric>

namespace {

using xtd::threading::Mutex;
using xtd::threading::SharedMutex;
using xtd::threading::Thread;
using xtd::containers::Vector;
using xtd::memory::UniquePtr;

class MutexTest : public ::testing::Test {};

// ---------------------------------------------------------------------
// Mutex
// ---------------------------------------------------------------------

    TEST_F(MutexTest, BasicFunctionality_BasicLoopIncrement) {
        constexpr uint32_t count = 1000;
        uint32_t counter = 0;
        Mutex m;
        {
            Vector<Thread> threads;
            threads.resize(count);
            for(size_t i = 0; i < count; ++i) {
                threads[i].create([&m, &counter](){
                    for(size_t i = 0; i < count; ++i) {
                        auto lock = m.getLock();
                        ++counter;
                    }
                });
            }
        }
        EXPECT_EQ(counter, count * count);
    }

    template<typename M>
    uint64_t runExclusionStress(uint32_t threadCount, uint32_t iters) {
        M m;
        std::atomic<int> inside{0};
        std::atomic<uint64_t> violations{0};
        uint64_t counter = 0;   // deliberately non-atomic
        {
            Vector<Thread> threads(threadCount);
            for (auto& t : threads) {
                t.create([&] {
                    for (uint32_t i = 0; i < iters; ++i) {
                        auto lock = m.getLock();
                        if (inside.fetch_add(1, std::memory_order_relaxed) != 0) ++violations;
                        uint64_t tmp = counter;              // split read-modify-write
                        if ((i & 31) == 0) Thread::yield();
                        counter = tmp + 1;
                        inside.fetch_sub(1, std::memory_order_relaxed);
                    }
                });
            }
        }
        EXPECT_EQ(counter == uint64_t(threadCount) * iters, violations == 0); // consistent
        return violations;
    }

    TEST_F(MutexTest, MutualExclusion) {
        EXPECT_EQ(runExclusionStress<Mutex>(16, 50'000), 0u);
    }

    // Test the test: a lock that does nothing MUST be caught
    struct NoopMutex { struct L{}; L getLock() { return {}; } };
    TEST_F(MutexTest, StressTest_DetectsBrokenMutex) {
        EXPECT_GT(runExclusionStress<NoopMutex>(16, 50'000), 0u);
    }

    TEST_F(MutexTest, Contender_BlocksWhileHeld) {
        Mutex m;
        std::atomic<bool> holding{false}, release{false}, acquired{false};

        Thread holder, contender;
        holder.create([&] {
            auto lock = m.getLock();
            holding = true;
            while (!release) Thread::yield();
        });
        while (!holding) Thread::yield();

        contender.create([&] {
            auto lock = m.getLock();
            acquired = true;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_FALSE(acquired);      // still blocked

        release = true;
        holder.join();
        contender.join();
        EXPECT_TRUE(acquired);       // and wakes up once released
    }

    TEST_F(MutexTest, Invariant_TwoFieldsStayEqual) {
        Mutex m;
        uint64_t a = 0, b = 0;
        std::atomic<uint64_t> torn{0};
        {
            Vector<Thread> threads(8);
            for (size_t t = 0; t < threads.size(); ++t) {
                threads[t].create([&, t] {
                    for (uint32_t i = 0; i < 50'000; ++i) {
                        auto lock = m.getLock();
                        if (a != b) ++torn;
                        if (t % 2 == 0) {            // writers
                            ++a;
                            if ((i & 15) == 0) Thread::yield();
                            ++b;
                        }
                    }
                });
            }
        }
        EXPECT_EQ(torn, 0u);
    }

} // namespace