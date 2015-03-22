#ifndef __COMMON_TEXT_STRINGTOOLS_HPP__
#define __COMMON_TEXT_STRINGTOOLS_HPP__

#include <common/common.hpp>
#include <string>

namespace ml { 
namespace common { 
namespace text {
	
class StringTools
{
public:
	static bool startsWith(const char *src, const char *token);
	static bool startsWith(const std::string &src, const char *token);
	static bool startsWith(const std::string &src, const std::string &token);

    static bool endsWith(const char * src, const char * token);
    static bool endsWith(const std::string & src, const std::string & token);

	static bool isDate(const char *str);
	static bool isDouble(const char *str, double &result);
	static bool isInteger(const char *str, int radix, int &result);
	static bool isInteger64(const char *str, int radix, int64 &result);
	static bool isDate(const std::string &str);
	static bool isDouble(const std::string &str, double &result);
	static bool isInteger(const std::string &str, int radix, int &result);
	static bool isInteger64(const std::string &str, int radix, int64 &result);

	static std::string loadTextFile(const std::string &filename);
	
	static std::string toLowerCase(const std::string &str);
	static std::string toUpperCase(const std::string &str);
	
	static std::string trim(
        const std::string & src,
        const std::string & whiteSpaces = " \n\r");
	static std::string trimRightSpaces(const std::string& src);
    static std::string trimChars(const std::string &src, char remove);
    static std::string trimLeadingChars(const std::string &src, char remove);
    static std::string trimTrailingChars(const std::string &src, char remove);

	static std::string removeChar(const std::string& str, char remove);
	static std::string removeChars(const std::string& str, const std::string& remove);
	static std::string removeNewlines(const std::string& str);
	
	static std::string replaceAll(const std::string& src, const std::string& orig, const std::string& dest);
	
	static void TestHarness();
};

}; // text
}; // common
}; // ml

#endif // __COMMON_TEXT_STRINGTOOLS_HPP__
