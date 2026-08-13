#pragma once
#include <source_location>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <format>

namespace xtd::debug {
    constexpr void alwaysVerify(bool expression, const char* exprString, const char* message, std::source_location loc = std::source_location::current()) {
        if(expression) return;
        std::fprintf(stderr, "Runtime verification \"%s\" failed at %s:%d in %s\n, \"%s\"", 
            exprString, loc.file_name(), loc.line(), loc.function_name(), message);
        std::abort();
    }

    constexpr void require(bool expression, const char* exprString, const char* message, std::source_location loc = std::source_location::current()) {
        if(expression) return;
        throw std::logic_error(std::format("Requirement \"{}\" not met at {}:{} in {}, \"{}\"", 
            exprString, loc.file_name(), loc.line(), loc.function_name(), message));
    }

    constexpr void verify(bool expression, const char* exprString, const char* message, std::source_location loc = std::source_location::current()) {
#ifndef NDEBUG
        if(expression) return;
        std::fprintf(stderr, "Verification \"%s\" failed at %s:%d in %s\n, \"%s\"", 
            exprString, loc.file_name(), loc.line(), loc.function_name(), message);
        std::abort();
#endif
    }
}

#define XTD_DEBUG_ALWAYS_VERIFY(expr, msg)    (::xtd::debug::alwaysVerify((expr), #expr, msg, std::source_location::current()))
#define XTD_DEBUG_REQUIRE(expr, msg)          (::xtd::debug::require((expr), #expr, msg, std::source_location::current()))

#ifndef NDEBUG
#define XTD_DEBUG_VERIFY(expr, msg)           (::xtd::debug::verify((expr), #expr, msg, std::source_location::current()))
#else
#define XTD_DEBUG_VERIFY(expr, msg)           ((void)0)
#endif

