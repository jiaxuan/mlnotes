#include "StringTools.hpp"

// #include <common/exception/Exception.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <ctype.h>
#include <cmath>

using namespace std;

namespace bl = boost::lambda;


bool StringTools::startsWith(const char *src, const char *token)
{
	int i = 0;
	while (src[i] && token[i])
	{
		// If there is a character mismatch, false
		if (src[i] != token[i])
		{
			return false;
		}
		
		++i;
	}
	
	// If we have reached the end of the token, we have a start match
	if (!token[i])
	{
		return true;
	}
	
	// If reached the end of the src, then we do not have a match
	return false;
}

bool StringTools::startsWith(const std::string &src, const char *token)
{
	return startsWith(src.c_str(), token);
}

bool StringTools::startsWith(const std::string &src, const std::string &token)
{
	return startsWith(src.c_str(), token.c_str());
}

bool StringTools::endsWith(const char * src, const char * suffix)
{
    size_t srcLen = ::strlen(src);
    size_t suffixLen = ::strlen(suffix);

    if (srcLen < suffixLen)
        return false;

    return strncmp(src + srcLen - suffixLen, suffix, suffixLen) == 0;
}

bool StringTools::endsWith(const std::string & src, const std::string & suffix)
{
    return endsWith(src.c_str(), suffix.c_str());
}

/**
 * Matches strings of the form 19yymmdd or 2yyymmdd
 */

bool StringTools::isDate(const char *str)
{
	// Check that the string is the correct length
	if (8 != strlen(str))
	{
		return false;
	}
	
	// Check that it comprises solely of digits
	for (int i = 0; i < 8; ++i)
	{
		if (str[i] < '0' || str[i] > '9')
		{
			return false;
		}
	}

	// Check 19yymmdd or 2yyymmdd
	if (!(str[0] == '1' && str[1] == '9' || str[0] == '2'))
	{
		return false;
	}

	// NB: We could add a lot more checks like valid month numbers, days in month, etc, but it is probably not worth the performance hit
	
	// If we get this far then it qualifies as a date
	return true;
}

bool StringTools::isDouble(const char *str, double &result)
{
	char *endptr = 0;
	
	result = strtod(str, &endptr);

	// If nothing was converted	
	if (0 == result && str == endptr)
	{
		return false;
	}
	
	// If the entire string was not a number
	if (0 != endptr && 0 != *endptr)
	{
		return false;
	}
	
	// If there was an overflow
	if (HUGE_VAL == result || -HUGE_VAL == result)
	{
		return false;
	}

	// If there was an underflow
	if (0 == result && ERANGE == errno)
	{
		return false;
	}

	// If we get this far, it qualifies
	return true;
}


bool StringTools::isInteger(const char *str, int radix, int &result)
{
	char *endptr = 0;
	
	result = strtol(str, &endptr, radix);

	// If nothing was converted	
	if (0 == result && str == endptr)
	{
		return false;
	}
	
	// If the entire string was not a number
	if (0 != endptr && 0 != *endptr)
	{
		return false;
	}
	
	// If there was an overflow
	if (ERANGE == errno && (LONG_MAX == result || LONG_MIN == result))
	{
		return false;
	}

	// If there was an error
	if (0 == result && ERANGE == errno)
	{
		return false;
	}

	// If we get this far, it qualifies
	return true;
}


bool StringTools::isInteger64(const char *str, int radix, int64 &result)
{
	char *endptr = 0;
	
	result = strtoll(str, &endptr, radix);

	// If nothing was converted	
	if (0 == result && str == endptr)
	{
		return false;
	}
	
	// If the entire string was not a number
	if (0 != endptr && 0 != *endptr)
	{
		return false;
	}
	
	// If there was an overflow
	if (ERANGE == errno && (LONG_MAX == result || LONG_MIN == result))
	{
		return false;
	}

	// If there was an error
	if (0 == result && ERANGE == errno)
	{
		return false;
	}

	// If we get this far, it qualifies
	return true;
}

bool StringTools::isDate(const std::string &str)
{
	return isDate(str.c_str());
}

bool StringTools::isDouble(const std::string &str, double &result)
{
	return isDouble(str.c_str(), result);
}

bool StringTools::isInteger(const std::string &str, int radix, int &result)
{
	return isInteger(str.c_str(), radix, result);
}

bool StringTools::isInteger64(const std::string &str, int radix, int64 &result)
{
	return isInteger64(str.c_str(), radix, result);
}


std::string StringTools::loadTextFile(const std::string &filename)
{
	ifstream is(filename.c_str());
	ostringstream oss;
	
	// Check that we managed to open the file
	if (!is.is_open())
	{
		// FTHROW(ml::common::exception::Exception, "Error reading file %s", filename.c_str());
		throw("Error reading file");
	}
	
	string line;
	while (getline(is, line))
	{
		oss << line << endl;
	}
	
	return oss.str();
}

std::string StringTools::toLowerCase(const std::string &str)
{
	// Is there a better way?!
	
	ostringstream oss;
	for (const char *c = str.c_str(); *c; ++c)
	{
		oss << (char) tolower(*c);
	}
	
	return oss.str();
}

std::string StringTools::toUpperCase(const std::string &str)
{
	// Is there a better way?!
	
	ostringstream oss;
	for (const char *c = str.c_str(); *c; ++c)
	{
		oss << (char) toupper(*c);
	}
	
	return oss.str();	
}

std::string StringTools::trim(
    const std::string & src,
    const std::string & whiteSpaces)
{
    size_t start = src.find_first_not_of(whiteSpaces);
    if (start == std::string::npos)
        return std::string();

    size_t end = src.find_last_not_of(whiteSpaces);

    // If string does not have to be trimmed
    if (start == 0 && end == src.size() - 1)
    {
        return src;
    }
    else
    {
        return src.substr(start, end - start + 1);
    }
}

string StringTools::trimRightSpaces(const string& src)
{
	const char *buffer = src.c_str();
	const int src_length = src.length();

	// Find the end of the string
	int end = src_length - 1;
	while ((0 <= end) && (' ' == buffer[end]))
	{
		--end;
	}
	
	// If string does not have to be trimmed
	if ((src_length-1) == end)
	{
		return src;
	}
	else
	{
		return src.substr(0, end+1);
	}
}

string StringTools::trimLeadingChars(const std::string &src, char remove)
{
    const char *buffer = src.c_str();
    const int src_length = src.length();
    
    // Find the start of the string
    int start = 0;
    while ( start < src_length && remove == buffer[start])
    {
        ++start;
    }
    
    // If string does not have to be trimmed
    if (0 == start)
    {
        return src;
    }
    else
    {
        return src.substr(start, src.length()-start);
    }
}

string StringTools::trimTrailingChars(const std::string &src, char remove)
{
    const char *buffer = src.c_str();
    const int src_length = src.length();
    
    // Find the end of the string
    int end = src_length - 1;
    while ((0 <= end) && ((remove == buffer[end])))
    {
        --end;
    }
    
    // If string does not have to be trimmed
    if ((src_length-1) == end)
    {
        return src;
    }
    else
    {
        return src.substr(0, end+1);
    }
}

string StringTools::trimChars(const std::string &src, char remove)
{
    return trimTrailingChars( trimLeadingChars( src, remove), remove);
}

string StringTools::removeChar(const std::string& str, char remove)
{
	string result;
	remove_copy(str.begin(), str.end(), back_inserter(result), remove);
	return result;
}

string StringTools::removeChars(const std::string& str, const std::string& remove)
{
	string result;
	remove_copy_if(str.begin(), str.end(), back_inserter(result), boost::is_any_of(remove));
	return result;
}

string StringTools::removeNewlines(const std::string& str)
{
	string result;
	remove_copy_if(str.begin(), str.end(), back_inserter(result),
		(bl::_1 == '\n') || (bl::_1 == '\r'));
	return result;
}

string StringTools::replaceAll(const std::string& src, const std::string& orig, const std::string& dest)
{
	string result = src;
	string::size_type j = 0;
	while ((j = result.find(orig, j)) != string::npos)
	{
		result.replace(j, orig.length(), dest);
		j += dest.length();
	}
	return result;
}

// ------------------------------------------------------------------------------------------------------------------------

void StringTools::TestHarness()
{
	cout << "StringTools testharness" << endl;
	
	cout << "toUpperCase Tests:" << endl;
	{ string str = "Hello there"; cout << str << ":" << toUpperCase(str) << endl; }
	{ string str = "Hello There"; cout << str << ":" << toUpperCase(str) << endl; }
	{ string str = ""; cout << str << ":" << toUpperCase(str) << endl; }
	{ string str = "2344 234 423 234"; cout << str << ":" << toUpperCase(str) << endl; }
	{ string str = "There is 1 other thing 123"; cout << str << ":" << toUpperCase(str) << endl; }
	
	
	cout << "Testing StringTools::startsWith()" << endl;
	cout << "true " << "=" << StringTools::startsWith("James was here", "James") << endl;
	cout << "false" << "=" << StringTools::startsWith("James", "James was here") << endl;
	cout << "false" << "=" << StringTools::startsWith("James was here", "Kames") << endl;
	cout << "true " << "=" << StringTools::startsWith("James was here", "") << endl;
	cout << "false" << "=" << StringTools::startsWith("", "James") << endl;
	cout << "true" << "=" << StringTools::startsWith("", "") << endl;
	
	cout << "Testing StringTools::isDate()" << endl;
	cout << "true " << "=" << StringTools::isDate("20000101") << endl;
	cout << "true " << "=" << StringTools::isDate("21000101") << endl;
	cout << "true " << "=" << StringTools::isDate("20999999") << endl;
	cout << "true " << "=" << StringTools::isDate("19999999") << endl;
	cout << "true " << "=" << StringTools::isDate("19000000") << endl;

	cout << "false" << "=" << StringTools::isDate("20abcdef") << endl;
	cout << "false" << "=" << StringTools::isDate("19abcdef") << endl;
	cout << "false" << "=" << StringTools::isDate("18000101") << endl;
	cout << "false" << "=" << StringTools::isDate("2345678a") << endl;
	cout << "false" << "=" << StringTools::isDate("12fhjshe") << endl;
	cout << "false" << "=" << StringTools::isDate("") << endl;
	cout << "false" << "=" << StringTools::isDate("990101") << endl;

	cout << "Testing StringTools::isDouble()" << endl;
	double value;
	cout << "true " << "=" << StringTools::isDouble("0", value) << endl;
	cout << "0" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("000000", value) << endl;
	cout << "0" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("00.0000", value) << endl;
	cout << "0" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("1000", value) << endl;
	cout << "1000" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("100000", value) << endl;
	cout << "100000" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("-1000", value) << endl;
	cout << "-1000" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("10000000", value) << endl;
	cout << "-10000000" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("1e4", value) << endl;
	cout << "1e4" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("-1.57575e7", value) << endl;
	cout << "-1.57575e7" << "=" << value << endl;
	cout << "true " << "=" << StringTools::isDouble("348563485683475683568345687345", value) << endl;
	cout << "true " << "=" << StringTools::isDouble("0.00000000000000000000000000000000000000000000000056", value) << endl;
	
	cout << "false" << "=" << StringTools::isDouble("", value) << endl;
	cout << "false" << "=" << StringTools::isDouble("12a", value) << endl;
	cout << "false" << "=" << StringTools::isDouble("aaa", value) << endl;
	cout << "false" << "=" << StringTools::isDouble("834765873468575e683456875", value) << endl;
	cout << "false" << "=" << StringTools::isDouble("+34975349759e9387", value) << endl;
	cout << "false" << "=" << StringTools::isDouble("-34975349759e9387", value) << endl;
	cout << "false" << "=" << StringTools::isDouble("0.00000010000000000000000000000000000000000000000056e-433", value) << endl;
}
