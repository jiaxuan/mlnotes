#ifndef __COMMON_TEXT_FORMAT_HPP__
#define __COMMON_TEXT_FORMAT_HPP__

#include <cstdarg>
#include "common.hpp"
#include "DecimalDecNumber.hpp"
#include <string>
#include <cstdarg>
#include <boost/noncopyable.hpp>

/** ----------------------------------------------------------------------------------
 */
class Format : private virtual boost::noncopyable
{
public:

	/**
	 * @param val the string to process
	 * @return the boolean value of <code>val</code>
	 * @throw InvalidArgumentException if <code>val</code> is NULL or invalid
	 */
	static bool stringToBool(const char *val);
	
	/**
	 * @param val the string to process
	 * @return the boolean value of <code>val</code>, or defaultValue if <code>val</code> is NULL or invalid
	 */
	static bool stringToBool(const char *val, bool defaultValue);

	/**
	 * @param val the boolean to format
	 * @return the boolean in string format yes/no
	 * @throw RuntimeException on unexpected error
	 */
	static std::string boolToString(bool val);

	/**
	 * @param val the boolean to format
	 * @return the boolean in string format Y/N
	 * @throw RuntimeException on unexpected error
	 */
	static std::string boolToStringYN(bool val);
	
	/**
	 * @param val the string to process
	 * @return the int32 value of <code>val</code> or zero if the string is invalid
	 * @throw InvalidArgumentException if <code>val</code> is NULL
	 */
	static int32 stringToInt32(const char *val);

	/**
	 * @param val the string to process
	 * @return the int32 value of <code>val</code>, or defaultValue if <code>val</code> is NULL or invalid
	 */
	static int32 stringToInt32(const char *val, int32 defaultValue);

	/**
	 * @param val the int32 to format
	 * @return the int32 in string format
	 * @throw RuntimeException on unexpected error
	 */
	static std::string int32ToString(int32 val);


	/**
	 * @param val the string to process
	 * @return the int64 value of <code>val</code> or zero if the string is invalid
	 * @throw InvalidArgumentException if <code>val</code> is NULL
	 */
	static int64 stringToInt64(const char *val);

	/**
	 * @param val the string to process
	 * @return the int64 value of <code>val</code>, or defaultValue if <code>val</code> is NULL or invalid
	 */
	static int64 stringToInt64(const char *val, int64 defaultValue);

	/**
	 * @param val the int64 to format
	 * @return the int64 in string format
	 * @throw RuntimeException on unexpected error
	 */
	static std::string int64ToString(int64 val);

	/**
	 * @param val the double number for format
    * @param dp the number of decimal places to keep
    * @param group whether to add a ',' for every 3 digits
	 * @return the double in string format
	 */
	static std::string doubleToString(double val, int dp = 2, bool group = true);

	/**
	 * @param val the string to process
	 * @return the double value of <code>val</code>, or defaultValue if <code>val</code> is NULL or invalid
	 */
    static double stringToDouble(const char *val);
	static double stringToDouble(const char *val, double defaultValue);

	/**
	 * Format a string using printf style syntax
	 * @param format the string format to use
	 * @return the formatted string
	 * @throw InvalidArgumentException if <code>format</code> is NULL
	 * @throw RuntimeException on unexpected error
	 */
	static std::string toString(const char *format, ...)
#ifdef __GNUC__	
		// Tell the compiler that this follows printf syntax so it can do
		// some static format and argument checking. 
		__attribute__ ((format (printf, 1, 2)))
#endif
	;
		
    /**
     * Format a string using printf style syntax
     * @param format the string format to use
     * @return the formatted string
     * @throw InvalidArgumentException if <code>format</code> is NULL
     * @throw RuntimeException on unexpected error
     */
    static std::string toStringV(const char *format, va_list ap);

	// static std::string StringArrayToString(const ml::common::text::StringArray &v);

	/**
	 * Format raw bytes as a hex string
	 * @param data, bytes to represent as a hext string
	 * @param dataSize, the number of bytes in the sequence data
	 * @return the formatted string
	 */
	 static	std::string toHexString( const char* data, unsigned int dataSize);
	
	/**
	 * Format raw bytes as a friendly trace format
	 * @param data, bytes to represent as a trace string
	 * @param dataSize, the number of bytes in the sequence data
	 * @return the formatted string
	 */
	 static	std::string toTraceString(const char* data, unsigned int dataSize);

	/**
	 * Format a 4bit value as a character 0-F
	 * @param c	the 4 bit value
	 * @return the character 0-F
	 */
	static char nibbleToChar( unsigned char c);

	/**
	 * Format decimal sting as a comma seperated
	 * @param value, the decimal string
	 * @return the formatted string
	 */
	static std::string toCommaFormatted(const std::string& value);
	static std::string toCommaFormatted(const DecimalDecNumber& value);
    
private:
	Format();
	~Format();
};

#endif // __COMMON_TEXT_FORMAT_HPP__
