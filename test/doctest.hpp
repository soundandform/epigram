#ifndef __doctest_hpp

//#	define DOCTEST_CONFIG_SUPER_FAST_ASSERTS

#	include <doctest/doctest.h>
#	define test_suite(NAME) TEST_SUITE(#NAME)
#	define doctest(NAME) TEST_CASE(NAME)

#	define doctest_disabled__(LINE) void DisabledTest##LINE ()
#	define doctest_disabled_(LINE) doctest_disabled__(LINE)
#	define doctest_disabled(NAME) doctest_disabled_(__LINE__)

#	define subcase(...) SUBCASE(__VA_ARGS__)
#	define expect(...) CHECK(__VA_ARGS__);

#	define DOCTEST_BREAK_INTO_DEBUGGER() __builtin_debugtrap ()

#include <iostream>
using namespace std;

#define __doctest_hpp
#endif

#if 0
	// https://github.com/doctest/doctest/blob/master/doc/markdown/commandline.md

	// useful doctest flags
	--test-case=Name.*

	--no-colors			// Xcode ain't colored
	--no-version

#endif
