#ifndef NEARLIGHTER_TEST_SUPPORT_H
#define NEARLIGHTER_TEST_SUPPORT_H

#include <nearlighter/math/vec3f.h>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string_view>

namespace nearlighter::test {

/**
 * Collects independent expectation failures within one test executable.
 *
 * Expectations remain active in every build configuration, unlike the standard
 * assert macro, and report enough numeric context to diagnose regressions.
 */
class Context {
public:
    /** Records a failure when condition is false. */
    void expectTrue(bool condition, std::string_view message) {
        if (!condition) fail(message);
    }

    /** Records a failure when condition is true. */
    void expectFalse(bool condition, std::string_view message) {
        expectTrue(!condition, message);
    }

    /** Compares floating-point scalars with an absolute tolerance. */
    void expectNear(float actual, float expected, float tolerance,
                    std::string_view message) {
        if (std::fabs(actual - expected) <= tolerance) return;

        std::cerr << "FAILED: " << message
                  << " (expected " << expected
                  << ", actual " << actual
                  << ", tolerance " << tolerance << ")\n";
        ++failure_count_;
    }

    /** Compares all vector components with an absolute tolerance. */
    void expectVecNear(const Vec3f& actual, const Vec3f& expected,
                       float tolerance, std::string_view message) {
        const bool matches =
            std::fabs(actual.x() - expected.x()) <= tolerance &&
            std::fabs(actual.y() - expected.y()) <= tolerance &&
            std::fabs(actual.z() - expected.z()) <= tolerance;
        if (matches) return;

        std::cerr << "FAILED: " << message
                  << " (expected " << expected
                  << ", actual " << actual
                  << ", tolerance " << tolerance << ")\n";
        ++failure_count_;
    }

    /** Prints the suite summary and returns a process exit code for CTest. */
    int finish(std::string_view suite_name) const {
        if (failure_count_ == 0) {
            std::cout << suite_name << ": all checks passed\n";
            return 0;
        }

        std::cerr << suite_name << ": " << failure_count_
                  << " check(s) failed\n";
        return 1;
    }

private:
    void fail(std::string_view message) {
        std::cerr << "FAILED: " << message << '\n';
        ++failure_count_;
    }

    std::size_t failure_count_ = 0;
};

}  // namespace nearlighter::test

#endif  // NEARLIGHTER_TEST_SUPPORT_H
