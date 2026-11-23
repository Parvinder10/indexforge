#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <cmath>
#include <sstream>

namespace test_framework {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void add_test(const std::string& name, std::function<void()> func) {
        tests_.push_back({name, std::move(func)});
    }

    const std::vector<TestCase>& tests() const { return tests_; }

private:
    std::vector<TestCase> tests_;
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRegistry::instance().add_test(name, std::move(func));
    }
};

class AssertionException : public std::exception {
public:
    explicit AssertionException(std::string msg) : msg_(std::move(msg)) {}
    const char* what() const noexcept override { return msg_.c_str(); }
private:
    std::string msg_;
};

inline void assert_true(bool cond, const std::string& expr, const char* file, int line) {
    if (!cond) {
        std::ostringstream ss;
        ss << file << ":" << line << " - Assertion failed: (" << expr << ") is false";
        throw AssertionException(ss.str());
    }
}

template<typename T, typename U>
inline void assert_eq(const T& a, const U& b, const std::string& expr_a, const std::string& expr_b, const char* file, int line) {
    if (!(a == b)) {
        std::ostringstream ss;
        ss << file << ":" << line << " - Assertion failed: " << expr_a << " (" << a << ") == "
           << expr_b << " (" << b << ")";
        throw AssertionException(ss.str());
    }
}

inline void assert_near(double a, double b, double eps, const std::string& expr_a, const std::string& expr_b, const char* file, int line) {
    if (std::abs(a - b) > eps) {
        std::ostringstream ss;
        ss << file << ":" << line << " - Assertion failed: |" << expr_a << " (" << a << ") - "
           << expr_b << " (" << b << ")| <= " << eps << " (diff=" << std::abs(a - b) << ")";
        throw AssertionException(ss.str());
    }
}

} // namespace test_framework

#define INDEXFORGE_CONCAT_IMPL(x, y) x##y
#define INDEXFORGE_CONCAT(x, y) INDEXFORGE_CONCAT_IMPL(x, y)

#define TEST_CASE(name) \
    static void INDEXFORGE_CONCAT(test_func_, __LINE__)(); \
    static test_framework::TestRegistrar INDEXFORGE_CONCAT(registrar_, __LINE__)(name, INDEXFORGE_CONCAT(test_func_, __LINE__)); \
    static void INDEXFORGE_CONCAT(test_func_, __LINE__)()

#define ASSERT_TRUE(cond) test_framework::assert_true((cond), #cond, __FILE__, __LINE__)
#define ASSERT_FALSE(cond) test_framework::assert_true(!(cond), "!(" #cond ")", __FILE__, __LINE__)
#define ASSERT_EQ(a, b) test_framework::assert_eq((a), (b), #a, #b, __FILE__, __LINE__)
#define ASSERT_NEAR(a, b, eps) test_framework::assert_near((a), (b), (eps), #a, #b, __FILE__, __LINE__)
#define ASSERT_GT(a, b) test_framework::assert_true((a) > (b), #a " > " #b, __FILE__, __LINE__)
#define ASSERT_GE(a, b) test_framework::assert_true((a) >= (b), #a " >= " #b, __FILE__, __LINE__)
