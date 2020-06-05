#ifndef __doctest_hpp

#	include <doctest/doctest.h>
#	define test_suite(NAME) TEST_SUITE(#NAME)
#	define doctest(NAME) TEST_CASE(NAME)
#	define subcase(...) SUBCASE(__VA_ARGS__)
#	define expect(...) CHECK(__VA_ARGS__);

#include <iostream>
using namespace std;

#define __doctest_hpp
#endif

