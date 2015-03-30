
#include "Format.hpp"
// #include <common/exception/Exception.hpp>
#include "StringBuilder.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <sstream>
#include <locale>
#include <string>
#include <iomanip>
#include <string>

using namespace std;

namespace {
	
	bool getBoolValue(const char *val, bool & result )
	{
		if( 0 == strcasecmp(val, "yes") || 
			0 == strcasecmp(val, "y") ||
			0 == strcmp(val, "1") ||
			0 == strcasecmp(val, "true") )
		{
			result = true;
			return true;
		}
			
		if( 0 == strcasecmp(val, "no") || 
			0 == strcasecmp(val, "n") ||
			0 == strcmp(val, "0") ||
			0 == strcasecmp(val, "false") )
		{
			result = false;
			return true;
		}
		
		return false;
	}

}

// ----------------------------------------------------------------------------------- 
// ml::common::text::Format implementation
// ----------------------------------------------------------------------------------- 

bool Format::stringToBool(const char *val)
{
	if( NULL == val )
		// THROW(exception::InvalidArgumentException, "argument to Format is NULL");
		throw("argument to Format is NULL");

	bool result = false;
	
	if( ! ::getBoolValue(val, result) )
		// THROW(exception::InvalidArgumentException, "could not format the provided string: '" + std::string(val) + "'");
		throw("could not format the provided string: '" + std::string(val) + "'");

	return result;
}

/** ----------------------------------------------------------------------------------
 */
bool Format::stringToBool(const char *val, bool defaultValue)
{
	if( NULL == val )
		return defaultValue;

	bool result = false;
	
	if( ! ::getBoolValue(val, result) )
		return defaultValue;

	return result;
}

/** ----------------------------------------------------------------------------------
 */
std::string Format::boolToString(bool val)
{
	return std::string( val ? "yes" : "no" );
}

/** ----------------------------------------------------------------------------------
 */
std::string Format::boolToStringYN(bool val)
{
	return std::string( val ? "Y" : "N" );
}

/** ----------------------------------------------------------------------------------
 */
int32 Format::stringToInt32(const char *val)
{
	if( NULL == val )
		// THROW(exception::InvalidArgumentException, "argument to Format is NULL");
		throw("argument to Format is NULL");

	int32 result = 0;
	
	if( 1 != sscanf(val, "%d", &result) )
		// THROW(exception::InvalidArgumentException, "could not format the provided string: '" +  std::string(val) + "'");
		throw("could not format the provided string: '" +  std::string(val) + "'");

	return result;
}

/** ----------------------------------------------------------------------------------
 */
int32 Format::stringToInt32(const char *val, int32 defaultValue)
{
	if( NULL == val )
		return defaultValue;

	int32 result = 0;
	
	if( 1 != sscanf(val, "%d", &result) )
		return defaultValue;

	return result;
}

/** ----------------------------------------------------------------------------------
 */
std::string Format::int32ToString(int32 val)
{
	char buf[ 512 ] = {0};
	sprintf(buf, "%d", val);
	return std::string(buf);
}

/** ----------------------------------------------------------------------------------
 */
namespace {
class GroupedPunct : public std::numpunct<char>
{
	public:
		explicit GroupedPunct(size_t r = 0) : std::numpunct<char>(r) {}

	protected:
		string_type do_grouping() const { return "\003"; }
};
}

std::string Format::doubleToString(double val, int dp, bool group)
{
	std::ostringstream oss;

	if (group)
	{
		std::locale stdloc;
		std::locale loc(stdloc, new GroupedPunct);
		oss.imbue(loc);
	}

	oss << std::fixed << std::setprecision(dp) << val;
	return oss.str();
}

/** ----------------------------------------------------------------------------------
 */
double Format::stringToDouble(const char *val)
{
    if( NULL == val )
        // THROW(exception::InvalidArgumentException, "argument to Format is NULL");
        throw("argument to Format is NULL");

    double result = 0;
    
    if( 1 != sscanf(val, "%lg", &result) )
        // THROW(exception::InvalidArgumentException, "could not format the provided string: '" +  std::string(val) + "'");
        throw("could not format the provided string: '" +  std::string(val) + "'");

    return result;
}

/** ----------------------------------------------------------------------------------
 */
double Format::stringToDouble(const char *val, double const defaultValue)
{
	if( NULL == val )
		return defaultValue;

	double result = 0;
    
    if( 1 != sscanf(val, "%lg", &result) )
        return defaultValue;

    return result;
}

/** ----------------------------------------------------------------------------------
 */
int64 Format::stringToInt64(const char *val)
{
	if( NULL == val )
		// THROW(exception::InvalidArgumentException, "argument to Format is NULL");
		throw("argument to Format is NULL");

	int64 result = 0;
	
	if( 1 != sscanf(val, "%ld", &result) )
		// THROW(exception::InvalidArgumentException, "could not format the provided string: '" +  std::string(val) + "'");
		throw("could not format the provided string: '" +  std::string(val) + "'");

	return result;
}

/** ----------------------------------------------------------------------------------
 */
int64 Format::stringToInt64(const char *val, int64 defaultValue)
{
	if( NULL == val )
		return defaultValue;

	int64 result = 0;
	
	if( 1 != sscanf(val, "%ld", &result) )
		return defaultValue;

	return result;
}

/** ----------------------------------------------------------------------------------
 */
std::string Format::int64ToString(int64 val)
{
	char buf[ 512 ] = {0};
	sprintf(buf, "%ld", val);
	return std::string(buf);
}

/** ----------------------------------------------------------------------------------
 */
std::string Format::toString(const char *format, ... )
{
    va_list ap;
    va_start(ap, format);

    if( NULL == format )
        // THROW(exception::InvalidArgumentException, "argument to Format is NULL");
        throw("argument to Format is NULL");

    int result = 0;

    if( (result = vsnprintf(NULL, 0, format, ap)) <= 0 )
    {
        // THROW(exception::RuntimeException, "error returned from vsnprintf", result);
        throw("error returned from vsnprintf", result);
    }

    std::string formattedString(result, '\0');
    result = vsnprintf(&formattedString[0], result+1, format, ap);
    va_end(ap);

    if( result <= 0 )
        // THROW(exception::RuntimeException, "error returned from vsnprintf", result);
        throw("error returned from vsnprintf", result);

    return formattedString;
}

/** ----------------------------------------------------------------------------------
*  */
std::string Format::toStringV(const char *format, va_list ap)
{
    if( NULL == format )
        // THROW(exception::InvalidArgumentException, "argument to Format is NULL");
        throw("argument to Format is NULL");

    int result = 0;
    std::string formattedString;

    if( (result = vsnprintf(NULL, 0, format, ap)) > 0 )
    {
        formattedString.resize(result);
        result = vsnprintf(&formattedString[0], result+1, format, ap);
    }

    if( result <= 0 )
        // THROW(exception::RuntimeException, "error returned from vsnprintf", result);
        throw("error returned from vsnprintf", result);

    return formattedString;
}

/** ----------------------------------------------------------------------------------
 */ 
// std::string Format::StringArrayToString(const ml::common::text::StringArray &v)
// {
// 	std::ostringstream ss;
// 	
// 	foreach( i , v )
// 	{
// 		ss << *i << std::endl;
// 	}
// 	
// 	return ss.str();
// }

/** ----------------------------------------------------------------------------------
 */ 
std::string Format::toHexString(const char* data, unsigned int dataSize)
{
	std::string rtn;
	
	if( !data || !dataSize)
	{
		return "<EMPTY BUFFER>";
	}
	
	unsigned int rem = dataSize;
	
	for( const char* ptr=data; rem>0; ++ptr, --rem)
	{
		char nibbles[2];		
		nibbles[0] = (*ptr) >> 4;
		nibbles[1] = ((unsigned int) (*ptr) << 28) >> 28;

		if( ptr != data)
		{
			rtn += ", ";
		}

		char str[5];
		str[0] = '0';
		str[1] = 'x';
		str[2] = nibbleToChar( nibbles[0]);
		str[3] = nibbleToChar( nibbles[1]);
		str[4] = '\0';
		
		rtn += str;
		
		if( isprint( *ptr))
		{
			char str2[6];
			str2[0] = ' ';
			str2[1] = '(';
			str2[2] = *ptr;
			str2[3] = ')';
			str2[4] = ' ';
			str2[5] = '\0';
			
			rtn += str2;
		}	
	}
	
	return rtn;
}

/** ----------------------------------------------------------------------------------
 */ 
std::string Format::toTraceString(const char* data, unsigned int dataSize)
{
	if( !data || !dataSize)
		return "";
	
    StringBuilder builder;
    char hexbuf[16];
    char buf[1024];
    const char *pos = data;
    unsigned char c;
    unsigned char count = 0;
    const unsigned char LINE_SIZE = 16;
                                                                                                                                                                      
    for(unsigned int i=0; i<dataSize; ++i)
    {
        c = *pos;

        sprintf(hexbuf, "%02X ", c);
        builder << hexbuf;

        buf[count] = isprint(c) ? c : '.';
                                                                                                                                                                  
        count++;
        pos++;

        if( count == LINE_SIZE )
        {
                buf[count] = '\0';
                builder << "   " << buf << "\n";
                count = 0;
        }
    }
                                                                                                                                                                      
    if( count )
    {
        while( count < LINE_SIZE )
        {
                builder << "   ";
                buf[count++] = ' ';
        }
                                                                                                                                                                  
        buf[count] = '\0';
        builder << "   " << buf << "\n";
    }
    
    return builder.str();
}

/** ----------------------------------------------------------------------------------
 */ 
char Format::nibbleToChar(unsigned char c)
{
	if      (         c<10) return '0' + c;
	else if ( c>9  && c<16) return 'A' + c - 10;
	
	return '?';
}

/** ----------------------------------------------------------------------------------
 */ 
std::string Format::toCommaFormatted(const std::string& value)
{
	size_t pos = value.find('.');
	if (pos == std::string::npos)
		pos = value.length(); // could be integer form

	int step = 0;
	std::string result = value;
	for (int i=pos-1; i>0; i--)
	{
		if (!isdigit(result[i]))
			break;

		if (++step == 3)
		{
			result.insert(i, 1, ',');
			step = 0;
		}
	}

	return result;
}

std::string Format::toCommaFormatted(const DecimalDecNumber& value)
{
    return Format::toCommaFormatted(value.toString());
}
