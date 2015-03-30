
// do some magic to support INT64_C for the apr under gcc3.4.2+
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif //__STDC_CONSTANT_MACROS

#include <cstdio>
#include <time.h>
#include "Time.hpp"
// #include <common/exception/Exception.hpp>
#include <apr_time.h>
#include "StringTools.hpp"

namespace {

const char * FORMAT_STRINGS[] = {
		"%04d-%3s-%02d %02d:%02d:%02d.%06d",  // YYYYMMMDD_HHMMSSUU
		"%04d-%02d-%02d %02d:%02d:%02d.%06d", // YYYYMMDD_HHMMSSUU
		"%04d-%02d-%02d %02d:%02d:%02d.%03d", // YYYYMMDD_HHMMSSMM
		"%04d-%3s-%02d %02d:%02d:%02d",       // YYYYMMMDD_HHMMSS
		"%04d-%02d-%02d %02d:%02d:%02d",      // YYYYMMDD_HHMMSS
		"%02d-%3s-%04d %02d:%02d:%02d",       // DDMMMYYYY_HHMMSS ***
		"%04d-%3s-%02d",                      // YYYYMMMDD
		"%04d-%02d-%02d",                     // YYYYMMDD
		"%02d/%02d/%04d",                     // DDMMYYYY
		"%02d:%02d:%02d.%06d",                // HHMMSSUU
		"%02d:%02d:%02d.%03d",                // HHMMSSMM
		"%02d:%02d:%02d",                     // HHMMSS
		
		"%lu.%03lu",                          // SSS_MMM
		"%lu.%06lu",                          // SSS_UUU
		
		"%04d%02d%02d",                       // ISO_YYYYMMDD           ISO 8601 basic format (no separators)
		"%04d%02d%02d%02d%02d%02d",           // ISO_YYYYMMDDHHMMSS     ISO 8601 basic format (no separators)
		"%02d/%02d/%04d %02d:%02d:%02d.%03d", // MMDDYYYY_HHMMSSMM
		
		"%02d-%02d:%02d:%02d.%06d",           // OFFSET
		
		"%02d %3s %04d %02d:%02d:%02d %3s",  // RFC-822
		"%02d-%3s-%04d",                     // DDMMMYYYY
		"%04d-%02d-%02dT%02d:%02d:%02d",     // YYYYMMDDTHHMMSS
        "%04d%02d%02dT%02d%02d%02d",         // ISO_YYYYMMDDTHHMMSS

		"%04d-%02d-%02dT%02d:%02d:%02dZ",       // ISO_8601_SEC_UTC
		"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",  // ISO_8601_MSEC_UTC
		"%04d-%02d-%02dT%02d:%02d:%02d.%06dZ",  // ISO_8601_USEC_UTC
		"%04d%02d%02d-%02d:%02d:%02d.%03d",     // YYYYMMDD-HHMMSSMM with dash
		"%02d:%02d",                            // HHMM (without seconds)
		"%02d%3s%04d %02d:%02d:%02d",     // DDMMMYYYY HH:MM:SS format for sending to Seawolf/Sierra
		"%02d%3s%04d %02d:%02d:%02d (%s)",     // DDMMMYYYY HH:MM:SS format for sending to Seawolf/Sierra
		};

    const char * ISO_8601_SEC_UTC_PARSE_FMT =  "%04d-%02d-%02dT%02d:%02d:%02d%c";      // last char will be checked for 'Z'
    const char * ISO_8601_MSEC_UTC_PARSE_FMT = "%04d-%02d-%02dT%02d:%02d:%02d.%03d%c"; // last char will be checked for 'Z'
    const char * ISO_8601_USEC_UTC_PARSE_FMT = "%04d-%02d-%02dT%02d:%02d:%02d.%06d%c"; // last char will be checked for 'Z'


    int32 monthFromString(const char *sz)
    {
        if( NULL == sz )
            // THROW(exception::InvalidFormatException, "Month string is NULL");
            throw("Month string is NULL");
        
        int i = 0;
        do
        {
            if( 0 == strcasecmp(sz, apr_month_snames[i]) )
                return i;
        }
        while( ++i < 12 );
        
        // THROW(exception::InvalidFormatException, "Unrecognized month string: " + std::string(sz) );
        throw("Unrecognized month string: ");
    }


    void checkTimezoneFormatConsistency(Time::Format f, Time::Zone tz)
    {
        if ( (f == Time::ISO_8601_SEC_UTC || f == Time::ISO_8601_MSEC_UTC || f == Time::ISO_8601_USEC_UTC)
             && (tz != Time::UTC) )
        {
            throw("UTC timezone expected for specified format");
            // THROW(exception::InvalidArgumentException, "UTC timezone expected for specified format");
        }
    }
} // namespace

/** ----------------------------------------------------------------------------------
 */
const Time   Time::EPOCH(0,0);
const Time	 Time::DAY(24 * 60 * 60 * APR_USEC_PER_SEC);
const Time	 Time::HOUR(60 * 60 * APR_USEC_PER_SEC);
const Time	 Time::MINUTE(60 * APR_USEC_PER_SEC);
const Time	 Time::SECOND(APR_USEC_PER_SEC);

const time_t Time::SECONDS_PER_DAY 	= 24 * 60 * 60;
const int64  Time::USECS_PER_SECOND	= APR_USEC_PER_SEC;
const int64  Time::USECS_PER_MILLISECOND = APR_USEC_PER_SEC / 1000;
const int64	 Time::USECS_PER_MINUTE	= 60 * APR_USEC_PER_SEC;
const int64	 Time::USECS_PER_HOUR	= 60 * 60 * APR_USEC_PER_SEC;
const int64  Time::USECS_PER_DAY	= 24 * 60 * 60 * APR_USEC_PER_SEC;

const Time	 Time::MIN_TIME(0,0);
const Time   Time::MAX_TIME("20300101", ISO_YYYYMMDD);		// We need this bigger one day please!

Time::Time() : 
	m_time( apr_time_now() )
{
}

/** ----------------------------------------------------------------------------------
 */
Time::Time(const Time &t) : 
	m_time( t.m_time )
{
}

/** ----------------------------------------------------------------------------------
 */
Time::Time(const struct timeval &tv) :
	m_time( apr_time_make(tv.tv_sec, tv.tv_usec) )
{
}

/** ----------------------------------------------------------------------------------
 */
Time::Time(const int64 usecs) : 
	m_time( usecs )
{
}

/** ----------------------------------------------------------------------------------
 */
Time::Time(const time_t secs, const int32 usecs) : 
	m_time( apr_time_make(secs , usecs) )
{
}

/** ----------------------------------------------------------------------------------
 */
static void get_timezone(const Time::Zone& tz, std::string& sTimeZone)
{
	if( tz == Time::LOCAL )
	{
		const time_t  now = time(NULL);
		const struct tm tnow = *localtime(&now);
		char    buff[100];
		strftime( buff, sizeof buff, "%Z", &tnow );
		sTimeZone = buff;
	}
	else
	{
		sTimeZone = "GMT";
	}
}

/** ----------------------------------------------------------------------------------
 */
Time::Time(const std::string &str, Time::Format f, Time::Zone tz) :
	m_time(0L)
{
    checkTimezoneFormatConsistency(f, tz);

	#define SCAN_TIME(str, format, count, ...) \
			if( count != (res = sscanf(str.c_str(), format, __VA_ARGS__)) ) \
				throw("Unrecognized time format string : ")
	
	if( str.size() == 0 )
		throw("Constructing a Time object with an empty format string");
		// THROW(exception::InvalidFormatException, "Constructing a Time object with an empty format string");

	int res = 0;

	if( f == SSS_MMM || f == SSS_UUU )
	{
		struct timeval tv;
		SCAN_TIME(str, FORMAT_STRINGS[f], 2, 
				  &tv.tv_sec, 
				  &tv.tv_usec);

		if( f == SSS_MMM )
		    tv.tv_usec *= 1000;

		m_time = apr_time_make(tv.tv_sec, tv.tv_usec);
		return;
	}

	if( f == OFFSET )
	{
		int32 days, hours, minutes, seconds, usecs;

		SCAN_TIME(str, FORMAT_STRINGS[f], 5,
				  &days,
				  &hours,
				  &minutes,
				  &seconds,
				  &usecs);

		m_time = (days * USECS_PER_DAY) +
				 (hours * USECS_PER_HOUR) +
				 (minutes * USECS_PER_MINUTE) +
				 (seconds * USECS_PER_SECOND) +
				 usecs;
				 
		return;
	}
	
	Time now;
	apr_time_exp_t et;

  if( tz == LOCAL )
  {
  	apr_time_exp_lt(&et, now.m_time);
  }
  else
  {
  	apr_time_exp_gmt(&et, now.m_time);
  }

	char buf[TIMEBUF_SIZE] = {0};
	char timezone[TIMEBUF_SIZE] = {0};

    char Z = 'x';
	
	// Perform the string scan
	switch( f )
	{
	case YYYYMMMDD_HHMMSSUU:
		SCAN_TIME(str, FORMAT_STRINGS[f], 7,
				  &et.tm_year,
				  buf,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec);

		et.tm_year -= 1900;
		et.tm_mon = monthFromString(buf);
		break;
		
	case YYYYMMDD_HHMMSSUU:
		SCAN_TIME(str, FORMAT_STRINGS[f], 7,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec);

		et.tm_year -= 1900;
		et.tm_mon -= 1;
		break;

	case YYYYMMDD_HHMMSSMM:
		SCAN_TIME(str, FORMAT_STRINGS[f], 7,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec);

		et.tm_usec *= 1000;
		et.tm_year -= 1900;
		et.tm_mon -= 1;
		break;
				
	case YYYYMMMDD_HHMMSS:
		SCAN_TIME(str, FORMAT_STRINGS[f], 6,
				  &et.tm_year,
				  buf,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec);

        et.tm_usec = 0;
		et.tm_year -= 1900;
		et.tm_mon = monthFromString(buf);
		break;

	case YYYYMMDD_HHMMSS:
		SCAN_TIME(str, FORMAT_STRINGS[f], 6,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec);

        et.tm_usec = 0;
		et.tm_year -= 1900;
		et.tm_mon -= 1;
		break;
	
	case DDMMMYYYY_HHMMSS_TZ_NODASH:
	{
    std::string myTimeZone;
    get_timezone(tz, myTimeZone);

		SCAN_TIME(str, FORMAT_STRINGS[f], 7,
				  &et.tm_mday,
				  buf,
				  &et.tm_year,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  myTimeZone.c_str());

		et.tm_year -= 1900;
		et.tm_mon = monthFromString(buf);
		et.tm_usec = 0;
		break;
	}
	case DDMMMYYYY_HHMMSS_NODASH:	
	case DDMMMYYYY_HHMMSS:
		SCAN_TIME(str, FORMAT_STRINGS[f], 6,
				  &et.tm_mday,
				  buf,
				  &et.tm_year,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec);

		et.tm_year -= 1900;
		et.tm_mon = monthFromString(buf);
		et.tm_usec = 0;
		break;

	case DDMMMYYYY:
		SCAN_TIME(str, FORMAT_STRINGS[f], 3,
				  &et.tm_mday,
				  buf,
				  &et.tm_year);

		et.tm_year -= 1900;
		et.tm_mon = monthFromString(buf);
		break;

	case YYYYMMMDD:
		SCAN_TIME(str, FORMAT_STRINGS[f], 3,
				  &et.tm_year,
				  buf,
				  &et.tm_mday);

		et.tm_year -= 1900;
		et.tm_mon = monthFromString(buf);
		break;
		
	case YYYYMMDD:
		SCAN_TIME(str, FORMAT_STRINGS[f], 3,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday);

		et.tm_year -= 1900;
		et.tm_mon -= 1;
		break;
		
	case DDMMYYYY:
		SCAN_TIME(str, FORMAT_STRINGS[f], 3,
				  &et.tm_mday,
				  &et.tm_mon,
				  &et.tm_year);

		et.tm_year -= 1900;
		et.tm_mon -= 1;
		break;
		
	case HHMMSSUU:
		SCAN_TIME(str, FORMAT_STRINGS[f], 4,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec);
		break;
		
	case HHMMSSMM:
		SCAN_TIME(str, FORMAT_STRINGS[f], 4,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec);
	
		et.tm_usec *= 1000;
		break;
		
	case HHMMSS:
		SCAN_TIME(str, FORMAT_STRINGS[f], 3,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec);
				  
		et.tm_usec = 0;
		break;

	case ISO_YYYYMMDD:
	
		if (str.length() != 8)
		{
			// FTHROW(exception::InvalidFormatException, "ISO date [%s] not in format YYYYMMDD", str.c_str());
			throw("ISO date [] not in format YYYYMMDD");
		}
		
		SCAN_TIME(str, FORMAT_STRINGS[f], 3,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday);

		et.tm_year -= 1900;
		et.tm_mon -= 1;
		break;

	case ISO_YYYYMMDDHHMMSS:
		SCAN_TIME(str, FORMAT_STRINGS[f], 6,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec);

		et.tm_year -= 1900;
		et.tm_mon -= 1;
		et.tm_usec = 0;
		break;
	
    case ISO_YYYYMMDDTHHMMSS:
        SCAN_TIME(str, FORMAT_STRINGS[f], 6,
                  &et.tm_year,
                  &et.tm_mon,
                  &et.tm_mday,
                  &et.tm_hour,
                  &et.tm_min,
                  &et.tm_sec);

        et.tm_year -= 1900;
        et.tm_mon -= 1;
        et.tm_usec = 0;
        break;
    
	case MMDDYYYY_HHMMSSMM:
		SCAN_TIME(str, FORMAT_STRINGS[f], 7,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_year,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec);

		et.tm_usec *= 1000;
		et.tm_year -= 1900;
		et.tm_mon -= 1;
		break;

	case RFC_822:
		SCAN_TIME(str, FORMAT_STRINGS[f], 7 ,
		          &et.tm_mday,
		          buf,
				  &et.tm_year,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  timezone);

        et.tm_usec = 0;
		et.tm_year -= 1900;
		et.tm_mon = monthFromString(buf);
		if(strncmp(timezone, "GMT", 3) != 0 )
		{
			// THROW(exception::InvalidFormatException, "Timezone should be GMT for RFC_822 format" +   std::string(timezone));
			throw("Timezone should be GMT for RFC_822 format");
		}
		break;
		
	case ISO_8601_SEC_UTC:
		SCAN_TIME(str, ISO_8601_SEC_UTC_PARSE_FMT, 7,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
                  &Z);

        et.tm_usec = 0;
		et.tm_year -= 1900;
		et.tm_mon -= 1;

        if (Z != 'Z')
            // THROW(exception::InvalidFormatException, "Expecting trailing 'Z' character for UTC tz");
            throw("Expecting trailing 'Z' character for UTC tz");
		break;

	case ISO_8601_MSEC_UTC:
		SCAN_TIME(str, ISO_8601_MSEC_UTC_PARSE_FMT, 8,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec,
                  &Z);

		et.tm_usec *= 1000;
		et.tm_year -= 1900;
		et.tm_mon -= 1;

        if (Z != 'Z')
            // THROW(exception::InvalidFormatException, "Expecting trailing 'Z' character for UTC tz");
            throw("Expecting trailing 'Z' character for UTC tz");
		break;

	case ISO_8601_USEC_UTC:
		SCAN_TIME(str, ISO_8601_USEC_UTC_PARSE_FMT, 8,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec,
                  &Z);

		et.tm_year -= 1900;
		et.tm_mon -= 1;

        if (Z != 'Z')
            // THROW(exception::InvalidFormatException, "Expecting trailing 'Z' character for UTC tz");
            throw("Expecting trailing 'Z' character for UTC tz");
		break;
	//added to support OSE time format
	case YYYYMMDD_HHMMSSMM_WITHDASH:
		SCAN_TIME(str, FORMAT_STRINGS[f], 7,
				  &et.tm_year,
				  &et.tm_mon,
				  &et.tm_mday,
				  &et.tm_hour,
				  &et.tm_min,
				  &et.tm_sec,
				  &et.tm_usec);

		et.tm_usec *= 1000;
		et.tm_year -= 1900;
		et.tm_mon -= 1;
		break;

	case HHMM:
		SCAN_TIME(str, FORMAT_STRINGS[f], 2,
				  &et.tm_hour,
				  &et.tm_min);
				  
		et.tm_sec = 0;
		et.tm_usec = 0;
		break;

	default:
		// THROW(exception::InvalidFormatException, "Unrecognized time format");
		throw("Unrecognized time format");
	}
	
	if( APR_SUCCESS != (res = apr_time_exp_gmt_get(&m_time, &et)) )
		// FTHROW(exception::InvalidFormatException, "Could not convert exploded time to compact time : %d", res);
		throw("Could not convert exploded time to compact time");
}

/** ----------------------------------------------------------------------------------
 */
int64 Time::usecs() const
{
	return m_time;
}

int Time::getDayOfWeek(const int timezoneOffset) const
{
	apr_time_exp_t exploded;  
	apr_status_t result = apr_time_exp_tz(&exploded, (apr_time_t) m_time, timezoneOffset*60);

	if( APR_SUCCESS != result)
	{
		// THROW(exception::Exception, "There was an exception while exploding our internal time");
		throw("There was an exception while exploding our internal time");
	}

	return exploded.tm_wday;
}


/** ----------------------------------------------------------------------------------
 */
Time::operator struct timeval() const
{
	struct timeval tv;
	tv.tv_sec = apr_time_sec(m_time);
	tv.tv_usec = apr_time_usec(m_time);
	return tv;
}

/** ----------------------------------------------------------------------------------
 */
Time & Time::operator=(const Time &t)
{
	m_time = t.m_time;
	return *this;
}

/** ----------------------------------------------------------------------------------
 */
Time & Time::operator+=(const Time &t)
{
	m_time += t.m_time;
	return *this;
}

/** ----------------------------------------------------------------------------------
 */
Time & Time::operator-=(const Time &t)
{
	m_time -= t.m_time;
	return *this;
}

/** ----------------------------------------------------------------------------------
 */
bool Time::operator==(const Time &t) const
{
	return m_time == t.m_time;
}

/** ----------------------------------------------------------------------------------
 */
bool Time::operator!=(const Time &t) const
{
	return m_time != t.m_time;
}

/** ----------------------------------------------------------------------------------
 */
bool Time::operator<(const Time &t) const
{
	return m_time < t.m_time;
}

/** ----------------------------------------------------------------------------------
 */
bool Time::operator<=(const Time &t) const
{
	return m_time <= t.m_time;
}

/** ----------------------------------------------------------------------------------
 */
bool Time::operator>(const Time &t) const
{
	return m_time > t.m_time;
}

/** ----------------------------------------------------------------------------------
 */
bool Time::operator>=(const Time &t) const
{
	return m_time >= t.m_time;
}

/** ----------------------------------------------------------------------------------
 */
Time Time::operator%(const Time& t) const
{
	return m_time % t.m_time;
}

/** ----------------------------------------------------------------------------------
 */
Time Time::round(TimeUnit unit, Time::Zone tz) const
{
	apr_time_exp_t et;

	if (tz == LOCAL)
		apr_time_exp_lt(&et, m_time);
	else
		apr_time_exp_gmt(&et, m_time);

	switch (unit)
	{
		// This switch deliberately falls through
		case WEEKS:
			et.tm_yday -= et.tm_wday;
			et.tm_mday -= et.tm_wday;
			et.tm_wday = 0;
		case DAYS:
			et.tm_hour = 0;
		case HOURS:
			et.tm_min = 0;
		case MINUTES:
			et.tm_sec = 0;
		case SECONDS:
			et.tm_usec = 0;
			break;
		default:
			// FTHROW(exception::InvalidFormatException, "invalid period");
			throw("invalid period");
	}
	
	int64 packedTime;	 
	int res = 0;

	if (APR_SUCCESS != (res = apr_time_exp_gmt_get(&packedTime, &et)))
	{
		throw("Could not convert exploded time to compact time");
		// FTHROW(exception::InvalidFormatException, "Could not convert exploded time to compact time : %d %d %d %d %d %d %d %d %d %d %d", 
		// 										  unit,
		// 										  tz,
		// 										  et.tm_year,
		// 										  et.tm_yday,
		// 										  et.tm_mday,
		// 										  et.tm_wday,
		// 										  et.tm_hour,
		// 										  et.tm_min,
		// 										  et.tm_sec,
		// 										  et.tm_usec,
		// 										  res
		// 										  );
	}
		
	return Time(packedTime);
}

/** ----------------------------------------------------------------------------------
 */
std::string Time::toLocalString() const
{
	return this->format(Time::DEFAULT, Time::LOCAL);
}

/** ----------------------------------------------------------------------------------
 */
std::string Time::toUTCString() const
{
	return this->format(Time::DEFAULT, Time::UTC);
}

/** ----------------------------------------------------------------------------------
 */
std::string Time::toISOStringUTC(Precision precision) const
{
    Format fmt = ISO_8601_USEC_UTC; 
    switch (precision)
    {
        case PREC_SECONDS:          fmt = ISO_8601_SEC_UTC;     break;
        case PREC_MILLISECONDS:     fmt = ISO_8601_MSEC_UTC;    break;
        case PREC_MICROSECONDS:     fmt = ISO_8601_USEC_UTC;    break;
    }
    return format(fmt, UTC); 
}

/** ----------------------------------------------------------------------------------
 */
std::string Time::format() const
{
	char buf[TIMEBUF_SIZE] = {0};
	return std::string( Time::format(*this, buf, Time::DEFAULT, Time::UTC) );
}

/** ----------------------------------------------------------------------------------
 */
std::string Time::format(Time::DurationFormat f) const
{
	char buf[TIMEBUF_SIZE] = {0};
	return std::string( Time::format(*this, buf, static_cast<Time::Format>(f), Time::UTC) );
}

/** ------------------------------------#include <ctime>----------------------------------------------
 */
std::string Time::format(Time::Format f, Time::Zone tz) const
{
	char buf[TIMEBUF_SIZE] = {0};
	return std::string( Time::format(*this, buf, f, tz) );
}

/** ----------------------------------------------------------------------------------
 */
const char * Time::format(const Time &t, char buf[TIMEBUF_SIZE], Time::Format f, Time::Zone tz )
{
    checkTimezoneFormatConsistency(f, tz);

	struct timeval tv;

	if( f == SSS_MMM || f == SSS_UUU )
	{
		tv = static_cast<struct timeval>(t);

		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 tv.tv_sec,
				 (f == SSS_MMM ? tv.tv_usec / 1000 : tv.tv_usec));
				 
		return buf;
	}
	
	if( f == OFFSET )
	{
		int64 tmp = t.m_time;
		int32 days = tmp / USECS_PER_DAY;
		tmp -= (int64(days) * USECS_PER_DAY);
		int32 hours = tmp / USECS_PER_HOUR;
		tmp -= (int64(hours) * USECS_PER_HOUR);
		int32 minutes = tmp / USECS_PER_MINUTE;
		tmp -= (int64(minutes) * USECS_PER_MINUTE);
		int32 seconds = tmp / USECS_PER_SECOND;
		tmp -= (int64(seconds) * USECS_PER_SECOND);
		int32 usecs = tmp;

		snprintf(buf, TIMEBUF_SIZE, FORMAT_STRINGS[f], days, hours, minutes, seconds, usecs);
				 
		return buf;	
	}
		
	apr_time_exp_t et;

	if( tz == LOCAL )
	{
		apr_time_exp_lt( &et, t.m_time );
	}
	else
	{
		apr_time_exp_gmt( &et, t.m_time );
	}

	switch( f )
	{
	case YYYYMMMDD_HHMMSSUU:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 apr_month_snames[et.tm_mon],
				 et.tm_mday,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec,
				 et.tm_usec);
		break;
	case DDMMMYYYY_HHMMSS_NODASH:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
                 et.tm_mday,
                 StringTools::toUpperCase(std::string(apr_month_snames[et.tm_mon])).c_str(),
                 1900 + et.tm_year,
                 et.tm_hour,
                 et.tm_min,
                 et.tm_sec);
	case DDMMMYYYY_HHMMSS_TZ_NODASH:
	{
		std::string timezone;
    get_timezone(tz, timezone);

		snprintf(buf,
				 TIMEBUF_SIZE,
				 FORMAT_STRINGS[f],
                 et.tm_mday,
                 StringTools::toUpperCase(std::string(apr_month_snames[et.tm_mon])).c_str(),
                 1900 + et.tm_year,
                 et.tm_hour,
                 et.tm_min,
                 et.tm_sec,
                 timezone.c_str());

		break;
	}
	case YYYYMMDD_HHMMSSUU:
	case ISO_8601_USEC_UTC:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 et.tm_mon + 1,
				 et.tm_mday,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec,
				 et.tm_usec);
		break;
	case YYYYMMDD_HHMMSSMM:
	case ISO_8601_MSEC_UTC:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 et.tm_mon + 1,
				 et.tm_mday,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec,
				 et.tm_usec/1000);
		break;
	case YYYYMMMDD_HHMMSS:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 apr_month_snames[et.tm_mon],
				 et.tm_mday,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec);
		break;
	case YYYYMMDD_HHMMSS:
	case ISO_8601_SEC_UTC:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 et.tm_mon + 1,
				 et.tm_mday,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec);
		break;
	case DDMMMYYYY_HHMMSS:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_mday,
				 apr_month_snames[et.tm_mon],
				 1900 + et.tm_year,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec);
		break;
	case DDMMMYYYY:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_mday,
				 apr_month_snames[et.tm_mon],
				 1900 + et.tm_year);
		break;
	case YYYYMMMDD:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 apr_month_snames[et.tm_mon],
				 et.tm_mday);
		break;
	case YYYYMMDD:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 et.tm_mon + 1,
				 et.tm_mday);
		break;
	case DDMMYYYY:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_mday,
				 et.tm_mon + 1,
				 1900 + et.tm_year);
		break;
	case HHMMSSUU:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec,
				 et.tm_usec);
		break;
	case HHMMSSMM:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec,
				 et.tm_usec / 1000);
		break;
	case HHMMSS:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec);
		break;
	case ISO_YYYYMMDD:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 et.tm_mon + 1,
				 et.tm_mday);
		break;
	case ISO_YYYYMMDDHHMMSS:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 et.tm_mon + 1,
				 et.tm_mday,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec);
		break;
	case MMDDYYYY_HHMMSSMM:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_mon + 1,
				 et.tm_mday,
				 1900 + et.tm_year,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec,
				 et.tm_usec/1000);
		break;
	case RFC_822:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_mday,
				 apr_month_snames[et.tm_mon],
				 1900 + et.tm_year,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec,
				 "GMT");
		break;
	case YYYYMMDDTHHMMSS:
	        snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 et.tm_mon + 1,
				 et.tm_mday,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec);
	        break;
    case ISO_YYYYMMDDTHHMMSS:
            snprintf(buf, 
                 TIMEBUF_SIZE, 
                 FORMAT_STRINGS[f],
                 1900 + et.tm_year,
                 et.tm_mon + 1,
                 et.tm_mday,
                 et.tm_hour,
                 et.tm_min,
                 et.tm_sec);
            break;

	case YYYYMMDD_HHMMSSMM_WITHDASH:
		snprintf(buf,
				 TIMEBUF_SIZE,
				 FORMAT_STRINGS[f],
				 1900 + et.tm_year,
				 et.tm_mon + 1,
				 et.tm_mday,
				 et.tm_hour,
				 et.tm_min,
				 et.tm_sec,
				 et.tm_usec/1000);
		break;

	case HHMM:
		snprintf(buf, 
				 TIMEBUF_SIZE, 
				 FORMAT_STRINGS[f],
				 et.tm_hour,
				 et.tm_min);
		break;

	default:
		// THROW(exception::InvalidFormatException, "Unrecognized time format");
		throw("Unrecognized time format");
	}
	
	return buf;
}

// ---------------------------------------------------------------------------------------------------------

void Time::addDays(const int delta)
{
	m_time += Time::USECS_PER_DAY * delta;
}	

void Time::addHours(const int delta)
{
	m_time += Time::USECS_PER_HOUR * delta;
}	

void Time::addMinutes(const int delta)
{
	m_time += Time::USECS_PER_MINUTE * delta;
}

void Time::addSeconds(const int delta)
{
	m_time += Time::USECS_PER_SECOND * delta;
}	

/** ----------------------------------------------------------------------------------
 */
int32 Time::diffAsDays(const Time& higher, const Time& lower)
{
	return static_cast<int32>( (higher.m_time - lower.m_time) / Time::USECS_PER_DAY );
}

/** ----------------------------------------------------------------------------------
 */
int64 Time::diffAsHours(const Time& higher, const Time& lower)
{
	return static_cast<int64>( (higher.m_time - lower.m_time) / Time::USECS_PER_HOUR );
}

/** ----------------------------------------------------------------------------------
 */
int64 Time::diffAsMinutes(const Time& higher, const Time& lower)
{
	return static_cast<int64>( (higher.m_time - lower.m_time) / Time::USECS_PER_MINUTE );
}

/** ----------------------------------------------------------------------------------
 */
int64 Time::diffAsSeconds(const Time& higher, const Time& lower)
{
	return static_cast<int64>( (higher.m_time - lower.m_time) / Time::USECS_PER_SECOND );
}

int64 Time::diffAsMillis(const Time& higher, const Time& lower)
{
        return static_cast<int64>( (higher.m_time - lower.m_time) / Time::USECS_PER_MILLISECOND );
}

/** ----------------------------------------------------------------------------------
 */
Time operator+(const Time &t1, const Time &t2)
{
	Time t3(t1);
	t3 += t2;
	return t3;
}

/** ----------------------------------------------------------------------------------
 */
Time operator-(const Time &t1, const Time &t2)
{
	Time t3(t1);
	t3 -= t2;
	return t3;
}

/** ----------------------------------------------------------------------------------
 */
std::ostream & operator<<(std::ostream &os, const Time &t)
{
	return os << t.format(Time::DEFAULT, Time::UTC);
}
