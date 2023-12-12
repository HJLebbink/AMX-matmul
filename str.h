#pragma once
#include <string>
#include <vector>

namespace utils::str
{
	static inline std::string remove_first_last(const std::string& str) {
		return (str.length() < 3) ? "" : str.substr(1, str.length() - 2);
	}

	static inline bool endsWith(const std::string& str, const std::string& suffix) {
		return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
	}

	static inline bool endsWith(const std::string& str, const char suffix) {
		return (str.size() >= 1) && (str[str.size() - 1] == suffix);
	}

	static inline std::string concat_path_file(const std::string& path, const std::string& filename) {
		return path + (utils::str::endsWith(path, '/') ? "" : "/") + filename;
	}

	static inline bool startsWith(const std::string& str, const std::string& prefix) {
		return (str.rfind(prefix, 0) == 0);
	}

	static inline bool startsWith(const std::string& str, const char prefix) {
		return (str.size() >= 1) && (str[0] == prefix);
	}

	static inline std::vector<std::string> split(const std::string& original, char separator) {
		std::vector<std::string> results;
		std::string::const_iterator start = original.begin();
		std::string::const_iterator end = original.end();
		std::string::const_iterator next = std::find(start, end, separator);
		while (next != end) {
			results.push_back(std::string(start, next));
			start = next + 1;
			next = std::find(start, end, separator);
		}
		results.push_back(std::string(start, next));
		return results;
	}

	// trim from start (in place)
	static inline void ltrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(ch);
			}));
	}

	// trim from end (in place)
	static inline void rtrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch);
			}).base(), s.end());
	}

	// trim from both ends (in place)
	static inline void trim(std::string& s) {
		ltrim(s);
		rtrim(s);
	}

	// trim from start (copying)
	static inline std::string ltrim_copy(std::string s) {
		ltrim(s);
		return s;
	}

	// trim from end (copying)
	static inline std::string rtrim_copy(std::string s) {
		rtrim(s);
		return s;
	}

	// trim from both ends (copying)
	static inline std::string trim_copy(std::string s) {
		trim(s);
		return s;
	}

	template <typename T>
	static inline std::string to_string_1000_sep(const T value, const char thousand_separator = '\'') {
		const std::string tmp = std::to_string(value);
		std::string result = "";
		const int length = static_cast<int>(tmp.length());
		for (int i = 1; i <= length; ++i) {
			const int pos = length - i;
			result = tmp[pos] + result;
			if ((i > 0) && (i < length) && ((i % 3) == 0)) result = thousand_separator + result;
			//std::cout << " to_string_1000_sep: tmp=" << tmp << "; pos=" << pos << "; i=" << i << "; tmp[pos]=" << tmp[pos] << "; result=" << result << std::endl;
		}
		return result;
	}
}
