#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <sstream>

#undef assert

namespace detail {
    static inline const std::string __RED__ = "\033[31m";
    static inline const std::string __GREEN__ = "\033[32m";
    static inline const std::string __YELLOW__ = "\033[33m";
    static inline const std::string __BLUE__ = "\033[34m";
    static inline const std::string __CYAN__ = "\033[36m";
    static inline const std::string __RESET__ = "\033[0m";
}

class TestFramework {
public:
    struct Result {
		double testDurationUs = 0.0;
		double fixtureSetupDurationUs = 0.0;
		double fixtureTeardownDurationUs = 0.0;
        std::string message = "";
    };

    struct TestInfo {
        std::string testName;
		std::function<Result()> testFunc;
	};

    struct BenchmarkResult {
		double avgTimeUs = 0.0;
		double medianTimeUs = 0.0;
		double minTimeUs = 0.0;
		double maxTimeUs = 0.0;
	};

    class Benchmark {
    private:
        std::string m_name;
        std::function<BenchmarkResult(Benchmark&)> m_benchmark;
        std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> m_startTimes;
        std::vector<double> m_durationTimes;
    public:
        Benchmark() = default;
        ~Benchmark() = default;

        Benchmark(const Benchmark&) = default;
        Benchmark& operator=(const Benchmark&) = default;

        Benchmark(Benchmark&&) = default;
        Benchmark& operator=(Benchmark&&) = default;

        Benchmark(std::function<BenchmarkResult(Benchmark&)> benchmark, size_t iterations, 
            const std::string& name) : m_name(name), m_benchmark(benchmark) {
            m_startTimes.resize(iterations);
            m_durationTimes.resize(iterations);
        }

        std::function<BenchmarkResult(Benchmark&)>& getBenchmark() {
            return m_benchmark;
        }

        BenchmarkResult getStatisticsData() {
            BenchmarkResult result;
            if (m_durationTimes.empty()) {
                return result;
            }
            std::sort(m_durationTimes.begin(), m_durationTimes.end());
            result.avgTimeUs = std::accumulate(m_durationTimes.begin(), m_durationTimes.end(), 0.0) / m_durationTimes.size();
            result.medianTimeUs = (m_durationTimes.size() % 2 == 0) ?
                (m_durationTimes[m_durationTimes.size() / 2 - 1] + m_durationTimes[m_durationTimes.size() / 2]) / 2.0 :
                m_durationTimes[m_durationTimes.size() / 2];
            result.minTimeUs = m_durationTimes.front();
            result.maxTimeUs = m_durationTimes.back();
            return result;
		}

        std::string printStatistics() {
			std::stringstream str;
            if (m_durationTimes.empty()) {
                str << "No benchmark data available.\n";
                return str.str();
            }

            std::sort(m_durationTimes.begin(), m_durationTimes.end());

            double avg = std::accumulate(m_durationTimes.begin(), m_durationTimes.end(), 0.0) / m_durationTimes.size();
            double median = (m_durationTimes.size() % 2 == 0) ?
                (m_durationTimes[m_durationTimes.size() / 2 - 1] + m_durationTimes[m_durationTimes.size() / 2]) / 2.0 :
                m_durationTimes[m_durationTimes.size() / 2];
            double min = m_durationTimes.front();
            double max = m_durationTimes.back();

            using namespace detail;
            str << "avg: " << __CYAN__ << avg << __RESET__ << " us, median: "
                << __CYAN__ << median << __RESET__ << " us, min: " << __CYAN__ << min
                << __RESET__ << " us, max: " << __CYAN__ << max << __RESET__ << " us\n";
            return str.str();
        }

        void setBenchmark(std::function<BenchmarkResult(Benchmark&)> benchmark, size_t iterations) {
            m_benchmark = benchmark;
            m_startTimes.resize(iterations);
            m_durationTimes.resize(iterations);
        }
        void startTiming(size_t i) {
            m_startTimes[i] = std::chrono::high_resolution_clock::now();
        }

        void endTiming(size_t i) {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> duration = end - m_startTimes[i];
            m_durationTimes[i] = duration.count();
        }

        const std::string& getName() const {
            return m_name;
		}

        size_t getIterations() const {
            return m_durationTimes.size();
		}
    };

    struct TestFileData {
        std::vector<TestInfo> testInfo;
        size_t nameColumnWidth;
    };

    struct BenchmarkFileData {
        std::vector<Benchmark> benchInfo;
        size_t nameColumnWidth;
        size_t iterationCountColumnWidth = sizeof("iterations: ");
    };

    using BenchmarkMap = std::map<std::string, BenchmarkFileData>;
    using BenchmarkIterator = std::vector<Benchmark>::iterator;

private:
    static inline std::map<std::string, TestFileData> m_tests;
    static inline std::map<std::string, std::shared_ptr<void>> m_sharedFixtures;
    static inline BenchmarkMap m_benchmarks;
    static inline size_t m_passedTests;
    static inline size_t m_failedTests;
	static inline const size_t s_avgDisplayPadding = 10;
    static inline const size_t s_medianDisplayPadding = 13;
    static inline const size_t s_maxDisplayPadding = 10;
    static inline const size_t s_minDisplayPadding = 10;

public:

    template<typename TestCallable>
    static Result runTest(TestCallable&& test) {
        using namespace detail;
        Result result;
        auto start = std::chrono::high_resolution_clock::now();
        try {			
            test();
			auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> duration = end - start;
            result.testDurationUs = duration.count();
            ++m_passedTests;
        }
        catch (const std::exception& e) {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> duration = end - start;
            result.testDurationUs = duration.count();
            ++m_failedTests;
            result.message = e.what();
        }
        return result;
    }

    // --- Basic test ---
    static void addTest(const std::string& filename, const std::string& name, std::function<void()> test) {
        auto& data = m_tests[filename];
        data.testInfo.emplace_back(name, [test]() {
            return runTest(test);
            });
        data.nameColumnWidth = std::max(data.nameColumnWidth, name.size());
    }

    // --- Parameterized test ---
    template<typename ParamVector, typename ParamFunction>
    static void addParameterizedTest(const std::string& filename, const std::string& name,
        ParamFunction&& test, const ParamVector& params)
    {
        auto& data = m_tests[filename];
        for (size_t i = 0; i < params.size(); ++i) {
			auto& param = params[i];
            data.testInfo.emplace_back(name + "[" + std::to_string(i) + "]",
                [test, param]() {
                    return runTest([&]() { test(param); });
                });
        }
        data.nameColumnWidth = std::max(data.nameColumnWidth, data.testInfo.back().testName.size());
    }

    // --- Fixture test ---
    template<typename FixtureType, typename TestFunction>
    static void addFixtureTest(const std::string& filename, const std::string& name, TestFunction&& test) {
        auto& data = m_tests[filename];
        data.testInfo.emplace_back(name, [test]() {
            return runTest([&]() {
                FixtureType fixture;
                test(fixture);
                });
            });
        data.nameColumnWidth = std::max(data.nameColumnWidth, name.size());
    }

    // --- Parameterized fixture test ---
    template<typename FixtureType, typename ParamVector, typename ParamFunction>
    static void addParameterizedFixtureTest(const std::string& filename, const std::string& name,
        ParamFunction&& test, const ParamVector& params)
    {
        auto& data = m_tests[filename];
        for (size_t i = 0; i < params.size(); ++i) {
            auto& param = params[i];
            data.testInfo.emplace_back(name + "[" + std::to_string(i) + "]",
                [test, param]() {
                    return runTest([&]() {
                        FixtureType fixture;
                        test(fixture, param);
                        });
                });
        }
        data.nameColumnWidth = std::max(data.nameColumnWidth, data.testInfo.back().testName.size());
    }

    // --- Shared fixture test ---
    template<typename FixtureType, typename TestFunction>
    static void addSharedFixtureTest(const std::string& filename, const std::string& fixtureName,
        const std::string& testName, TestFunction&& test)
    {
        auto& data = m_tests[filename];
        data.testInfo.emplace_back(testName, [test, filename, fixtureName]() {
            return runTest([&]() {
                auto key = filename + ":" + fixtureName;
                auto& fixturePtr = m_sharedFixtures[key];
                if (!fixturePtr) fixturePtr = std::make_shared<FixtureType>();
                test(*std::static_pointer_cast<FixtureType>(fixturePtr));
                });
            });
        data.nameColumnWidth = std::max(data.nameColumnWidth, testName.size());
    }

    // --- Parameterized shared fixture test ---
    template<typename FixtureType, typename ParamVector, typename ParamFunction>
    static void addParameterizedSharedFixtureTest(const std::string& filename, const std::string& fixtureName,
        const std::string& testName, ParamFunction&& test,
        const ParamVector& params)
    {
        auto& data = m_tests[filename];
        for (size_t i = 0; i < params.size(); ++i) {
            auto& param = params[i];
            data.testInfo.emplace_back(testName + "[" + std::to_string(i) + "]",
                [test, param, filename, fixtureName]() {
                    return runTest([&]() {
                        auto key = filename + ":" + fixtureName;
                        auto& fixturePtr = m_sharedFixtures[key];
                        if (!fixturePtr) fixturePtr = std::make_shared<FixtureType>();
                        test(*std::static_pointer_cast<FixtureType>(fixturePtr), param);
                        });
                });
        }
        data.nameColumnWidth = std::max(data.nameColumnWidth, data.testInfo.back().testName.size());
    }

    // --- Generated param test ---
    template<typename ParamFunction, typename ParamGenerator>
    static void addGeneratedParamTest(const std::string& filename, const std::string& name,
        ParamFunction&& test, ParamGenerator&& paramGenerator, size_t count)
    {
        auto& data = m_tests[filename];
        for (size_t i = 0; i < count; ++i) {
            data.testInfo.emplace_back(name + "[" + std::to_string(i) + "]",
                [test, paramGenerator, i]() {
                    return runTest([&]() { test(paramGenerator(i)); });
                });
        }
        data.nameColumnWidth = std::max(data.nameColumnWidth, data.testInfo.back().testName.size());
    }

    // --- Generated param fixture test ---
    template<typename FixtureType, typename ParamFunction, typename ParamGenerator>
    static void addGeneratedParamFixtureTest(const std::string& filename, const std::string& name,
        ParamFunction&& test, ParamGenerator&& paramGenerator, size_t count)
    {
        auto& data = m_tests[filename];
        for (size_t i = 0; i < count; ++i) {
            data.testInfo.emplace_back(name + "[" + std::to_string(i) + "]",
                [test, paramGenerator, i]() {
                    return runTest([&]() {
                        FixtureType fixture;
                        test(fixture, paramGenerator(i));
                        });
                });
        }
        data.nameColumnWidth = std::max(data.nameColumnWidth, data.testInfo.back().testName.size());
    }

    // --- Generated param shared fixture test ---
    template<typename FixtureType, typename ParamFunction, typename ParamGenerator>
    static void addGeneratedParamSharedFixtureTest(const std::string& filename, const std::string& fixtureName,
        const std::string& testName, ParamFunction&& test, ParamGenerator&& paramGenerator, size_t count)
    {
        auto& data = m_tests[filename];
        for (size_t i = 0; i < count; ++i) {
            data.testInfo.emplace_back(testName + "[" + std::to_string(i) + "]",
                [test, paramGenerator, i, filename, fixtureName]() {
                    return runTest([&]() {
                        auto key = filename + ":" + fixtureName;
                        auto& fixturePtr = m_sharedFixtures[key];
                        if (!fixturePtr) fixturePtr = std::make_shared<FixtureType>();
                        test(*std::static_pointer_cast<FixtureType>(fixturePtr),
                            paramGenerator(i));
                        });
                });
        }
        data.nameColumnWidth = std::max(data.nameColumnWidth, data.testInfo.back().testName.size());
    }

    static void addBenchmark(const std::string& filename, const std::string& name,
        std::function<void(Benchmark&, size_t)> benchmarkFunc, size_t iterations = 1000) {
        auto& fileData = m_benchmarks[filename];

        fileData.benchInfo.emplace_back([name, benchmarkFunc, iterations](Benchmark& benchmark) {
            for (size_t i = 0; i < iterations; ++i) {
                benchmarkFunc(benchmark, i);
            }
            BenchmarkResult result = benchmark.getStatisticsData();
            return result;
            }, iterations, name);
        fileData.nameColumnWidth = std::max(fileData.nameColumnWidth, name.size());
        fileData.iterationCountColumnWidth = std::max(fileData.iterationCountColumnWidth, std::to_string(iterations).size());
    }

    template<typename FixtureType>
    static void addFixtureBenchmark(const std::string& filename, const std::string& name,
        std::function<void(FixtureType&, Benchmark&, size_t)> benchmarkFunc, size_t iterations = 1000) {
        auto& fileData = m_benchmarks[filename];
        
        fileData.benchInfo.emplace_back([name, benchmarkFunc, iterations](Benchmark& benchmark) {
            FixtureType fixture;
            for (size_t i = 0; i < iterations; ++i) {
                benchmarkFunc(fixture, benchmark, i);
            }

            BenchmarkResult result = benchmark.getStatisticsData();
            return result;
            }, iterations, name);
        fileData.nameColumnWidth = std::max(fileData.nameColumnWidth, name.size());
        fileData.iterationCountColumnWidth = std::max(fileData.iterationCountColumnWidth, std::to_string(iterations).size());
    }

    template<typename ParamFunction, typename ParamVector>
    static void addParameterizedBenchmark(const std::string& filename, const std::string& name,
        ParamFunction&& benchmarkFunc, const ParamVector& params, size_t iterations = 1000) {
        size_t index = 0;
        auto& fileData = m_benchmarks[filename];

        for (const auto& param : params) {
            fileData.benchInfo.emplace_back([name, benchmarkFunc, param, iterations, index](Benchmark& benchmark) {
                for (size_t i = 0; i < iterations; ++i) {
                    benchmarkFunc(param, benchmark, i);
                }
                BenchmarkResult result = benchmark.getStatisticsData();
                return result;
                }, iterations, name + "[" + std::to_string(index) + "]");
            index++;
        }
        fileData.nameColumnWidth = std::max(fileData.nameColumnWidth, fileData.benchInfo.back().getName().size());
        fileData.iterationCountColumnWidth = std::max(fileData.iterationCountColumnWidth, std::to_string(iterations).size());
    }
    
    static inline void startTiming(Benchmark& benchmark, size_t iteration) {
        benchmark.startTiming(iteration);
    }
    
    static inline void endTiming(Benchmark& benchmark, size_t iteration) {
        benchmark.endTiming(iteration);
    }

    static void runAll() {
        runAllTests();
        std::cout << "\n";
        runAllBenchmarks();
    }

    static std::string formatPaddedString(const std::string& str, size_t width)
    {
        if (str.size() > width)
            return str;
		std::string result = str + std::string(width - str.size(), ' ');
		return result;
    }

    template<typename Numeric>
    static std::string formatPaddedString(const Numeric& num, size_t width)
    {
        std::stringstream stream;
        stream << num;
        auto str = stream.str();
        if (str.size() > width)
            return str;
        std::string result = str + std::string(width - str.size(), ' ');
        return result;
    }

    static void runAllTests() {
        using namespace detail;
        m_passedTests = 0;
        m_failedTests = 0;
        int totalTests = 0;

        for (const auto& [file, tests] : m_tests) {
            totalTests += tests.testInfo.size();
        }

        if (totalTests > 0) {
            std::cout << "Running " << __CYAN__ << totalTests << __RESET__ << " tests from " << __CYAN__ << m_tests.size() << __RESET__ << " files...\n\n";
            std::vector<std::pair<Result, size_t>> passes;
            std::vector<std::pair<Result, size_t>> fails;
            size_t timePadding = 0;

            for (const auto& [filename, tests] : m_tests) {
                std::cout << __YELLOW__ << "[" << filename << "]\n" << __RESET__;
				
                for (size_t i = 0; i < tests.testInfo.size(); ++i) {
                    auto result = tests.testInfo[i].testFunc();
                    std::stringstream str;
                    str << result.testDurationUs;
                    timePadding = std::max(timePadding, str.str().size());
                    if (result.message.size() == 0) {
						passes.emplace_back(result, i);
                    }
                    else {
                        fails.emplace_back(result, i);
                    }
                }

                for (size_t i = 0; i < passes.size(); ++i) {
                    std::cout << "  " << formatPaddedString(tests.testInfo[passes[i].second].testName, tests.nameColumnWidth)
                        << __GREEN__ << " PASSED" << __RESET__ << " (" << __CYAN__ <<
                        formatPaddedString(passes[i].first.testDurationUs, timePadding)
                        << __RESET__ << " us)\n";
                }
                for(size_t i = 0; i < fails.size(); ++i) {
                    std::cout << "  " << formatPaddedString(tests.testInfo[fails[i].second].testName, tests.nameColumnWidth)
                        << __RED__ << " FAILED" << __RESET__ << " (" << __CYAN__ <<
                        formatPaddedString(fails[i].first.testDurationUs, timePadding)
                        << __RESET__ << " us) " << " with exception: " << __YELLOW__ << fails[i].first.message
                        << __RESET__ << "\n";
                }

                std::cout << "\n  Results: " << __GREEN__ << passes.size() << __RESET__ << " passed, "
                    << __RED__ << fails.size() << __RESET__ << " failed\n";

                std::cout << "\n";
                passes.clear();
                fails.clear();
            }
            std::cout << "Results: " << __GREEN__ << m_passedTests << __RESET__ << " passed, "
                << __RED__ << m_failedTests << __RESET__ << " failed\n";
        }
        else {
            std::cout << "No tests found\n";
        }
    }

    static void runAllBenchmarks() {
        using namespace detail;
        int totalBenchmarks = 0;

        for (const auto& [file, benches] : m_benchmarks) {
            totalBenchmarks += benches.benchInfo.size();
        }

        if (totalBenchmarks > 0) {
            std::cout << "Running " << __CYAN__ << totalBenchmarks << __RESET__ << " benchmarks from "
                << __CYAN__ << m_benchmarks.size() << __RESET__ << " files...\n\n";            

            for (auto& [filename, benches] : m_benchmarks) {
                std::cout << __YELLOW__ << "[" << filename << "]\n" << __RESET__;
                std::cout << "  " << formatPaddedString("name:", benches.nameColumnWidth) << " "
                    << formatPaddedString("iterations: ", benches.iterationCountColumnWidth) << " "
                    << formatPaddedString("avg, us: ", s_avgDisplayPadding) << " "
                    << formatPaddedString("median, us: ", s_medianDisplayPadding) << " "
                    << formatPaddedString("min, us: ", s_minDisplayPadding) << " "
                    << formatPaddedString("max, us: ", s_maxDisplayPadding) << "\n";

                for (size_t i = 0; i < benches.benchInfo.size(); ++i) {
                    auto result = benches.benchInfo[i].getBenchmark()(benches.benchInfo[i]);
                    std::cout << "  " << formatPaddedString(benches.benchInfo[i].getName(), benches.nameColumnWidth) << " "
                        << __CYAN__ << formatPaddedString(benches.benchInfo[i].getIterations(), benches.iterationCountColumnWidth) << " "
                        << formatPaddedString(result.avgTimeUs, s_avgDisplayPadding) << " "
                        << formatPaddedString(result.medianTimeUs, s_medianDisplayPadding) << " "
                        << formatPaddedString(result.minTimeUs, s_minDisplayPadding) << " "
                        << formatPaddedString(result.maxTimeUs, s_maxDisplayPadding)
                        << __RESET__ << "\n";
                }
                std::cout << "\n";
            }
        }
        else {
            std::cout << "No benchmarks found\n";
        }
    }

    static void cleanup() {
        m_sharedFixtures.clear();
    }

    static void assert(bool condition, const std::string& message = "") {
        if (!condition) {
            throw std::runtime_error(message.empty() ? "Assertion failed" : message);
        }
    }

    template<typename T, typename U>
    static void assertEqual(const T& expected, const U& actual, const std::string& message = "") {
        if (expected != actual) {
            std::ostringstream oss;
            oss << "Expected: " << expected << ", Actual: " << actual;
            throw std::runtime_error(message.empty() ? oss.str() : message);
        }
    }

    template<typename T, typename U>
    static void assertNotEqual(const T& expected, const U& actual, const std::string& message = "") {
        if (expected == actual) {
            std::ostringstream oss;
            oss << "Expected " << expected << " != " << actual;
            throw std::runtime_error(message.empty() ? oss.str() : message);
        }
    }

    template<typename T, typename U>
    static void assertLessThan(const T& left, const U& right, const std::string& message = "") {
        if (left >= right) {
            std::ostringstream oss;
            oss << left << " >= " << right;
            throw std::runtime_error(message.empty() ? oss.str() : message);
        }
    }

    template<typename T, typename U>
    static void assertGreaterThan(const T& left, const U& right, const std::string& message = "") {
        if (left <= right) {
            std::ostringstream oss;
            oss << left << " <= " << right;
            throw std::runtime_error(message.empty() ? oss.str() : message);
        }
    }

    template<typename T>
    static void assertNear(const T& expected, const T& actual, const T& epsilon, const std::string& message = "") {
        if (std::abs(expected - actual) > epsilon) {
            std::ostringstream oss;
            oss << "Expected: " << expected << ", Actual: " << actual << ", Diff: " << std::abs(expected - actual) << " > " << epsilon;
            throw std::runtime_error(message.empty() ? oss.str() : message);
        }
    }

    template<typename T, typename U>
    static void assertLessEqual(const T& left, const U& right, const std::string& message = "") {
        if (left > right) {
            std::ostringstream oss;
            oss << left << " > " << right;
            throw std::runtime_error(message.empty() ? oss.str() : message);
        }
    }

    template<typename T, typename U>
    static void assertGreaterEqual(const T& left, const U& right, const std::string& message = "") {
        if (left < right) {
            std::ostringstream oss;
            oss << left << " < " << right;
            throw std::runtime_error(message.empty() ? oss.str() : message);
        }
    }

    template<typename T>
    static void assertNull(const T* ptr, const std::string& message = "") {
        if (ptr != nullptr) {
            throw std::runtime_error(message.empty() ?
                "Pointer is not null" : message);
        }
    }

    template<typename T>
    static void assertNotNull(const T* ptr, const std::string& message = "") {
        if (ptr == nullptr) {
            throw std::runtime_error(message.empty() ?
                "Pointer is null" : message);
        }
    }

    static void assertTrue(bool condition, const std::string& message = "") {
        if (!condition) {
            throw std::runtime_error(message.empty() ?
                "Expected true" : message);
        }
    }

    static void assertFalse(bool condition, const std::string& message = "") {
        if (condition) {
            throw std::runtime_error(message.empty() ?
                "Expected false" : message);
        }
    }

    template<typename Exception, typename Func>
    static void assertThrows(Func&& func, const std::string& message = "") {
        try {
            func();
            throw std::runtime_error(message.empty() ?
                "Expected exception was not thrown" : message);
        } catch (const Exception&) {
            // Expected exception caught
        }
    }

    template<typename Func>
    static void assertNoThrow(Func&& func, const std::string& message = "") {
        try {
            func();
        }
        catch (...) {
            throw std::runtime_error(message.empty() ?
                "Unexpected exception thrown" : message);
        }
    }

    class BenchmarkRAII {
        Benchmark& m_benchmark;
        size_t m_iteration;
    public:
        BenchmarkRAII(Benchmark& benchmark, size_t iteration) : m_benchmark(benchmark), m_iteration(iteration) {
            TestFramework::startTiming(benchmark, iteration);
        }
        ~BenchmarkRAII() {
            TestFramework::endTiming(m_benchmark, m_iteration);
        }
    };

    class BenchmarkScope {
        Benchmark& m_benchmark;
        size_t m_iteration;
    public:

        BenchmarkScope(Benchmark& benchmark, size_t iteration) : m_benchmark(benchmark), m_iteration(iteration) {
        }

        template <typename Func>
        BenchmarkScope& operator=(Func&& benchmark) {
            try {
                TestFramework::startTiming(m_benchmark, m_iteration);
                benchmark();
                TestFramework::endTiming(m_benchmark, m_iteration);
            }
            catch (...) {
                TestFramework::endTiming(m_benchmark, m_iteration);
                throw;
            }
            return *this;
        }
    };

};





#define __TEST__(name) \
    static void test_##name(); \
    static bool registered_##name = (TestFramework::addTest(std::filesystem::path(__FILE__).filename().string(), #name, test_##name), true); \
    static void test_##name()

#define __TEST_FIXTURE__(FixtureType, name) \
    static void test_fixture_##name(FixtureType& fixture); \
    static bool registered_fixture_##name = (TestFramework::addFixtureTest<FixtureType>(std::filesystem::path(__FILE__).filename().string(), #name, test_fixture_##name), true); \
    static void test_fixture_##name(FixtureType& fixture)

#define __TEST_SHARED_FIXTURE__(FixtureType, fixtureName, testName) \
    static void test_shared_##fixtureName##_##testName(FixtureType& fixture); \
    static bool registered_shared_##fixtureName##_##testName = (TestFramework::addSharedFixtureTest<FixtureType>(std::filesystem::path(__FILE__).filename().string(), #fixtureName, #testName, test_shared_##fixtureName##_##testName), true); \
    static void test_shared_##fixtureName##_##testName(FixtureType& fixture)

#define __TEST_PARAM__(name, ...) \
    static void test_param_auto_##name(const decltype(std::vector{__VA_ARGS__})::value_type& param); \
    static bool registered_param_auto_##name = (TestFramework::addParameterizedTest(std::filesystem::path(__FILE__).filename().string(), #name, test_param_auto_##name, std::vector{__VA_ARGS__}), true); \
    static void test_param_auto_##name(const decltype(std::vector{__VA_ARGS__})::value_type& param)

#define __TEST_FIXTURE_PARAM__(FixtureType, name, ...) \
    static void test_fixture_param_auto_##name(FixtureType& fixture, const decltype(std::vector{__VA_ARGS__})::value_type& param); \
    static bool registered_fixture_param_auto_##name = (TestFramework::addParameterizedFixtureTest<FixtureType>(std::filesystem::path(__FILE__).filename().string(), #name, test_fixture_param_auto_##name, std::vector{__VA_ARGS__}), true); \
    static void test_fixture_param_auto_##name(FixtureType& fixture, const decltype(std::vector{__VA_ARGS__})::value_type& param)

#define __TEST_SHARED_FIXTURE_PARAM__(FixtureType, fixtureName, testName, ...) \
    static void test_shared_param_auto_##fixtureName##_##testName(FixtureType& fixture, const decltype(std::vector{__VA_ARGS__})::value_type& param); \
    static bool registered_shared_param_auto_##fixtureName##_##testName = (TestFramework::addParameterizedSharedFixtureTest<FixtureType>(std::filesystem::path(__FILE__).filename().string(), #fixtureName, #testName, test_shared_param_auto_##fixtureName##_##testName, std::vector{__VA_ARGS__}), true); \
    static void test_shared_param_auto_##fixtureName##_##testName(FixtureType& fixture, const decltype(std::vector{__VA_ARGS__})::value_type& param)

#define __TEST_GENERATED_PARAM__(name, paramGenerator, paramCount) \
    static void test_generated_param_auto_##name(const decltype(std::invoke(paramGenerator, 0))& param); \
    static bool registered_generated_param_auto_##name = (TestFramework::addGeneratedParamTest(std::filesystem::path(__FILE__).filename().string(), #name, test_generated_param_auto_##name, paramGenerator, paramCount), true); \
    static void test_generated_param_auto_##name(const decltype(std::invoke(paramGenerator, 0))& param)

#define __TEST_FIXTURE_GENERATED_PARAM__(FixtureType, name, paramGenerator, paramCount) \
    static void test_fixture_generated_param_auto_##name(FixtureType& fixture, const decltype(std::invoke(paramGenerator, 0))& param); \
    static bool registered_fixture_generated_param_auto_##name = (TestFramework::addGeneratedParamFixtureTest<FixtureType>(std::filesystem::path(__FILE__).filename().string(), #name, test_fixture_generated_param_auto_##name, paramGenerator, paramCount), true); \
    static void test_fixture_generated_param_auto_##name(FixtureType& fixture, const decltype(std::invoke(paramGenerator, 0))& param)

#define __TEST_SHARED_FIXTURE_GENERATED_PARAM__(FixtureType, fixtureName, testName, paramGenerator, paramCount) \
    static void test_shared_generated_param_auto_##fixtureName##_##testName(FixtureType& fixture, const decltype(std::invoke(paramGenerator, 0))& param); \
    static bool registered_shared_generated_param_auto_##fixtureName##_##testName = (TestFramework::addGeneratedParamSharedFixtureTest<FixtureType>(std::filesystem::path(__FILE__).filename().string(), #fixtureName, #testName, test_shared_generated_param_auto_##fixtureName##_##testName, paramGenerator, paramCount), true); \
    static void test_shared_generated_param_auto_##fixtureName##_##testName(FixtureType& fixture, const decltype(std::invoke(paramGenerator, 0))& param)

#define __BENCHMARK__(name, ...) \
    static void benchmark_##name(TestFramework::Benchmark& __benchmark__, size_t __iteration__); \
    static bool registered_bench_##name = (TestFramework::addBenchmark(std::filesystem::path(__FILE__).filename().string(), #name, benchmark_##name, ##__VA_ARGS__), true); \
    static void benchmark_##name(TestFramework::Benchmark& __benchmark__, size_t __iteration__)

#define __BENCHMARK_FIXTURE__(FixtureType, name, ...) \
    static void benchmark_fixture_##name(FixtureType& fixture, TestFramework::Benchmark& __benchmark__, size_t __iteration__); \
    static bool registered_bench_fixture_##name = (TestFramework::addFixtureBenchmark<FixtureType>(std::filesystem::path(__FILE__).filename().string(), #name, benchmark_fixture_##name, ##__VA_ARGS__), true); \
    static void benchmark_fixture_##name(FixtureType& fixture, TestFramework::Benchmark& __benchmark__, size_t __iteration__)

#define __BENCHMARK_PARAM__(name, iterations, ...) \
    static void benchmark_param_##name(const decltype(std::vector{__VA_ARGS__})::value_type& param, TestFramework::Benchmark& __benchmark__, size_t __iteration__); \
    static bool registered_bench_param_##name = (TestFramework::addParameterizedBenchmark(std::filesystem::path(__FILE__).filename().string(), #name, benchmark_param_##name, std::vector{__VA_ARGS__}, iterations), true); \
    static void benchmark_param_##name(const decltype(std::vector{__VA_ARGS__})::value_type& param, TestFramework::Benchmark& __benchmark__, size_t __iteration__)

#define __BENCHMARK_START__() TestFramework::startTiming(__benchmark__, __iteration__)
#define __BENCHMARK_END__() TestFramework::endTiming(__benchmark__, __iteration__)
#define __BENCHMARK_RAII__() TestFramework::BenchmarkRAII __benchmark_RAII__(__benchmark__, __iteration__)
#define __BENCHMARK_SCOPE__() TestFramework::BenchmarkScope __benchmark_scope__(__benchmark__, __iteration__); \
    __benchmark_scope__= [&]() \

#define __ASSERT__(condition) TestFramework::assert(condition, #condition)
#define __ASSERT_EQ__(expected, actual) TestFramework::assertEqual(expected, actual)
#define __ASSERT_NE__(expected, actual) TestFramework::assertNotEqual(expected, actual)
#define __ASSERT_LT__(a, b) TestFramework::assertLessThan(a, b)
#define __ASSERT_GT__(a, b) TestFramework::assertGreaterThan(a, b)
#define __ASSERT_NEAR__(expected, actual, epsilon) TestFramework::assertNear(expected, actual, epsilon)
#define __ASSERT_LE__(a, b) TestFramework::assertLessEqual(a, b)
#define __ASSERT_GE__(a, b) TestFramework::assertGreaterEqual(a, b)
#define __ASSERT_NULL__(ptr) TestFramework::assertNull(ptr)
#define __ASSERT_NOT_NULL__(ptr) TestFramework::assertNotNull(ptr)
#define __ASSERT_TRUE__(condition) TestFramework::assertTrue(condition)
#define __ASSERT_FALSE__(condition) TestFramework::assertFalse(condition)
#define __ASSERT_THROWS__(exception, func) TestFramework::assertThrows<exception>(func)
#define __ASSERT_NO_THROW__(func) TestFramework::assertNoThrow(func)