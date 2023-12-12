#pragma once
//module;
#include <chrono>
#include <sstream>
#include <string>

#include "str.h"
#ifdef _MSC_VER		// compiler: Microsoft Visual Studio
#	include <windows.h>
#else
#	include <x86intrin.h>
#endif
#include <corecrt.h>
#include <time.h>
#include <iosfwd>
#include <ostream>
#define rdtsc __rdtsc

//export module tools.timing;

namespace tools::timing
{
	static inline unsigned long long timing_start;
	static inline unsigned long long timing_end;

	inline void reset_and_start_timer()
	{
		timing_start = rdtsc();
	}

	// Returns the number of millions of elapsed processor cycles since the last reset_and_start_timer() call.
	[[nodiscard]] inline double get_elapsed_mcycles()
	{
		timing_end = rdtsc();
		return static_cast<double>(timing_end - timing_start) / (1024. * 1024.);
	}

	// Returns the number of thousands of elapsed processor cycles since the last reset_and_start_timer() call.
	[[nodiscard]] inline double get_elapsed_kcycles()
	{
		timing_end = rdtsc();
		return static_cast<double>(timing_end - timing_start) / (1024.);
	}

	// Returns the number of elapsed processor cycles since the last reset_and_start_timer() call.
	[[nodiscard]] inline unsigned long long get_elapsed_cycles()
	{
		timing_end = rdtsc();
		return (timing_end - timing_start);
	}

	[[nodiscard]] inline std::string elapsed_cycles_str(const unsigned long long start, const unsigned long long end)
	{
		std::stringstream ss;
		const auto diff = end - start;
		ss << diff << " cycles = "
			<< diff / 1000 << " kcycles = "
			<< diff / 1000000 << " mcycles";
		return ss.str();
	}

	[[nodiscard]] inline std::string elapsed_time_str(const std::chrono::system_clock::time_point start, const std::chrono::system_clock::time_point end)
	{
		std::stringstream ss;
		const auto diff = end - start;
		ss << utils::str::to_string_1000_sep(std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()) << " ms = "
			<< utils::str::to_string_1000_sep(std::chrono::duration_cast<std::chrono::seconds>(diff).count()) << " sec = "
			<< utils::str::to_string_1000_sep(std::chrono::duration_cast<std::chrono::minutes>(diff).count()) << " min = "
			<< utils::str::to_string_1000_sep(std::chrono::duration_cast<std::chrono::hours>(diff).count()) << " hours" << std::endl;
		return ss.str();
	}

	[[nodiscard]] inline std::string current_time_str()
	{
		std::stringstream ss;
		time_t result = time(NULL);
		char str[26];
		ctime_s(str, sizeof str, &result);
		ss << str;
		return ss.str().substr(0, 24);
	}
}
