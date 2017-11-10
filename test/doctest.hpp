#ifndef __doctest_hpp

#	include "doctest.h"
#	define test_(...) TEST_CASE(__VA_ARGS__)
#	define test_suite(...) TEST_SUITE(__VA_ARGS__)
#	define doctest(NAME) TEST_CASE(#NAME)
#	define subcase(...) SUBCASE(__VA_ARGS__)
#	define expect(...) CHECK(__VA_ARGS__);

#include <iostream>
using namespace std;

#define __doctest_hpp
#endif

