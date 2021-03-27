#ifndef __doctest_hpp

#	include <doctest/doctest.h>
#	define test_suite(NAME) TEST_SUITE(#NAME)
#	define doctest(NAME) TEST_CASE(NAME)

#	define doctest_disabled__(LINE) void DisabledTest##LINE ()
#	define doctest_disabled_(LINE) doctest_disabled__(LINE)
#	define doctest_disabled(NAME) doctest_disabled_(__LINE__)

#	define subcase(...) SUBCASE(__VA_ARGS__)
#	define expect(...) CHECK(__VA_ARGS__);

#include <iostream>
using namespace std;

#define __doctest_hpp
#endif

