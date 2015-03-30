#ifndef __COMMON_UTIL_TIME_HPP__
#define __COMMON_UTIL_TIME_HPP__

#include "common.hpp"
#include <ostream>
#include <string>
#include <sys/time.h>

/** ------------------------------------------------------------------------------
 * @warning For the time being, this class only supports printing dates onwards from the
 * 			EPOCH value of 01-Jan-1970 00:00:00.000000
 */
class Time {
public:

	/**
	 * Supported time formats
	 */
	enum Format {
		YYYYMMMDD_HHMMSSUU = 0,		// <year>-<month name>-<day> <hour>:<minutes>:<seconds>.<microseconds>
		YYYYMMDD_HHMMSSUU  = 1,		// <year>-<month>-<day> <hour>:<minutes>:<seconds>.<microseconds>
		YYYYMMDD_HHMMSSMM  = 2,		// <year>-<month>-<day> <hour>:<minutes>:<seconds>.<milliseconds>
		YYYYMMMDD_HHMMSS   = 3,		// <year>-<month name>-<day> <hour>:<minutes>:<seconds>
		YYYYMMDD_HHMMSS    = 4,		// <year>-<month>-<day> <hour>:<minutes>:<seconds>
		DDMMMYYYY_HHMMSS   = 5,		// <day>-<month name>-<year> <hour>:<minutes>:<seconds>
		YYYYMMMDD          = 6,		// <year>-<month name>-<day>
		YYYYMMDD           = 7,		// <year>-<month>-<day>
		DDMMYYYY           = 8,		// <day>/<month>/<year>
		HHMMSSUU           = 9,		// <hour>:<minutes>:<seconds>.<microseconds>
		HHMMSSMM           = 10,	// <hour>:<minutes>:<seconds>.<milliseconds>
		HHMMSS             = 11,	// <hour>:<minutes>:<seconds>

		SSS_MMM            = 12,	// <seconds delta>.<milliseconds>
		SSS_UUU            = 13,	// <seconds delta>.<microseconds>
		
		ISO_YYYYMMDD       = 14,	// <year><month><day> (no dashes)
		ISO_YYYYMMDDHHMMSS = 15,	// <year><month><day><hour><minute><second> (no dashes)
		MMDDYYYY_HHMMSSMM  = 16, 	// <month>/<day>/<year> <hour>:<minute>:<second>:<milliseconds> eg."10/21/2004 20:00:00.440" 
		
		OFFSET			   = 17,	// <days>-<hours>:<minutes>:<seconds>.<microseconds>
		
		RFC_822			   = 18,	// <day> <month> <year> <hour>:<minutes>:<seconds> <GMT>
		DDMMMYYYY          = 19,	// <day>-<month name>-<year>
		YYYYMMDDTHHMMSS    = 20,    // <year>-<month name>-<day>T<hour>:<minutes>:<seconds>
        ISO_YYYYMMDDTHHMMSS = 21,   // <year><month name><day>T<hour><minutes><seconds> (no dashes/colons)

        ISO_8601_SEC_UTC   = 22,    // <year>-<month>-<day>T<hour>:<min>:<sec>Z
        ISO_8601_MSEC_UTC  = 23,    // <year>-<month>-<day>T<hour>:<min>:<sec>.<milliseconds>Z
        ISO_8601_USEC_UTC  = 24,    // <year>-<month>-<day>T<hour>:<min>:<sec>.<microseconds>Z
        YYYYMMDD_HHMMSSMM_WITHDASH = 25,		// <year><month name><day>-<hour>:<minutes>:<seconds>.<milliseconds>
		HHMM               = 26,    // <hour>:<minutes> (without seconds)
		DDMMMYYYY_HHMMSS_NODASH   = 27,    // <day><short month><year> <hour>:<minutes>:<seconds>, eg. "21OCT2004 20:00:00" used to send dates to Sierra
		DDMMMYYYY_HHMMSS_TZ_NODASH   = 28,    // <day><short month><year> <hour>:<minutes>:<seconds>, eg. "21OCT2004 20:00:00" used to send dates to Sierra
		DEFAULT = YYYYMMMDD_HHMMSSUU
	};

	enum DurationFormat {
		SSSMMM             = SSS_MMM,	// <seconds delta>.<milliseconds>
		SSSUUU             = SSS_UUU	// <seconds delta>.<microseconds>
	};

    enum Precision {
        PREC_SECONDS        = 0,
        PREC_MILLISECONDS   = 1,
        PREC_MICROSECONDS   = 2
    };
	
	enum {
		/** This should be large enough to handle all the supported formats */
		TIMEBUF_SIZE = 512
	};
	
	enum Zone {
		UTC,
		LOCAL
	};

	enum TimeUnit {
		SECONDS,
		MINUTES,
		HOURS,
		DAYS,
		WEEKS
	};

	static const Time   EPOCH;				// 1970-Jan-01 00:00:00.000000 UTC
	static const Time	DAY;				// A time representing 24 hours
	static const Time	HOUR;				// A time representing 1 hour
	static const Time	MINUTE;				// A time representing 1 minute
	static const Time	SECOND;				// A time representing 1 second
	static const time_t SECONDS_PER_DAY;            // Seconds in a day
        static const int64  USECS_PER_MILLISECOND;      // Microseconds in a millisecond;
	static const int64  USECS_PER_SECOND;           // Microseconds in a second
	static const int64	USECS_PER_MINUTE;	// Microseconds in a minute
	static const int64	USECS_PER_HOUR;		// Microseconds in a hour
	static const int64  USECS_PER_DAY;		// Microseconds in a day
	static const Time   MIN_TIME;			// The smallest time we can represent (epoch)
	static const Time   MAX_TIME;			// The largest time we can represent
	
	
public:

	/**
	 * Populates this object with the current local time
	 */
	Time();

	/**
	 * Copy constructor
	 */
	Time(const Time &t);

	/**
	 */
	Time(const struct timeval &tv);

	/**
	 * @param usecs the microseconds since Epoch value
	 */
	Time(const int64 usecs);

	/**
	 * @param secs the seconds since Epoch value
	 * @param usecs the microseconds past the last second in secs
	 */
	Time(const time_t secs, const int32 usecs);

	/**
	 * Construct a Time instance from a formatted string
	 * Throws an exception if the time is not correctly formatted
	 */
	Time(const std::string &str, Time::Format f = Time::DEFAULT, Time::Zone tz = Time::UTC);
	
	/**
	 * Return an int64 usecs since epoch
	 */
	int64 usecs() const;
	
	/**
	 * Returns the day of the week in the correct timezone.
	 * The timezoneOffset parameter is the number of minutes +GMT  (positive numbers for the east of GMT)
	 * Results are 0 == Sunday, 1 == Monday, etc.
	 */
	int getDayOfWeek(const int timezoneOffset) const;	

	/**
	 * Cast to a struct timeval
	 *
	 * struct timeval
	 * {
	 *		__time_t tv_sec;		// Seconds
	 *		__suseconds_t tv_usec;	// Microseconds
	 * };
	 */
	operator struct timeval() const;

	/**
	 */
	Time & operator=(const Time &t);

	/**
	 */
	Time & operator+=(const Time &t);

	/**
	 */
	Time & operator-=(const Time &t);
	
	/**
	 */
	bool operator==(const Time &t) const;

	/**
	 */
	bool operator!=(const Time &t) const;

	/**
	 */
	bool operator<(const Time &t) const;

	/**
	 */
	bool operator<=(const Time &t) const;

	/**
	 */
	bool operator>(const Time &t) const;

	/**
	 */
	bool operator>=(const Time &t) const;

	/**
	 */
	Time operator%(const Time &t) const;
	
	/**
	 * Adds a number of seconds (positive or negative) to the current time
	 */
	void addSeconds(const int delta);
	
	/**
	 * Adds a number of minutes (positive or negative) to the current time
	 */
	void addMinutes(const int delta);

	/**
	 * Adds a number of hours (positive or negative) to the current time
	 */
	void addHours(const int delta);
	
	/**
	 * Adds a number of days (positive or negative) to the current time
	 */
	void addDays(const int delta);
	
	/**
	 * Returns the time rounded to the specified units. For example, rounding the
	 * following date 10/03/2006 11:53:45.192 (a Friday) by:
	 * WEEK:	= 05/03/2006 00:00:00.000
	 * DAY:		= 10/03/2006 00:00:00.000
	 * HOUR:	= 10/03/2006 11:00:00.000
	 * MINUTE:	= 10/03/2006 11:53:00.000
	 * SECOND:	= 10/03/2006 11:53:45.000
	 * 
	 * @note assumes start of the week is Sunday 00:00:00
	 * @param unit one of WEEK, DAY, HOUR, MINUTE or SECOND.
	 * @param tz the timezone to use
	 * @return a Time representing just the date part of this time
	 */
	Time round(Time::TimeUnit unit, Time::Zone tz = UTC) const;
	
	/**
	 */
	std::string toLocalString() const;

	/**
	 */
	std::string toUTCString() const;

    /**
     *  ISO 8601 extended format:
     *    date values separator is '-', time values separator is ':'
     *    'T' separates time from date, 'Z' means UTC time zone (Zulu time)
     *  examples:
     *       2010-03-05T17:50:32Z           seconds precision
     *       2010-03-05T17:50:32.274Z       milliseconds precision
     *       2010-03-05T17:50:32.274932Z    microseconds precision
     */
    std::string toISOStringUTC(Precision precision = PREC_MICROSECONDS) const;

	/**
	 * Formats this <code>Time</code> object in the default format and UTC timezone.
	 * 
	 * @param f the format to use
	 * @return a string representation of this object
	 * @throw InvalidFormatException if <code>f</code> is not a recognized format
	 */
	std::string format() const;

	/**
	 * Formats this <code>Time</code> object in the requested timezone agnostic format
	 * 
	 * @param f the format to use
	 * @return a string representation of this object
	 * @throw InvalidFormatException if <code>f</code> is not a recognized format
	 */
	std::string format(Time::DurationFormat f) const;

	/**
	 * Formats this <code>Time</code> object in the requested format
	 * 
	 * @param f the format to use
	 * @return a string representation of this object
	 * @throw InvalidFormatException if <code>f</code> is not a recognized format
	 */
	std::string format(Time::Format f, Time::Zone tz) const;
		
	/**
	 * Formats the provided <code>Time</code> object in the requested format written to <code>buf</code>,
	 * then returns <code>buf</code>
	 * 
	 * @param t the <code>Time</code> to format
	 * @param buf the destination buffer in which to place the resulting format string
	 * @param f the format to use
	 * @return <code>buf</code>
	 * @throw InvalidFormatException if <code>f</code> is not a recognized format
	 */
	static const char *	format(const Time &t, char buf[TIMEBUF_SIZE], Time::Format f, Time::Zone tz);

	/**
	 * Formats the provided <code>Time</code> object in the requested format written to <code>buf</code>,
	 * then returns <code>buf</code>
	 * 
	 * @param t the <code>Time</code> to format
	 * @param buf the destination buffer in which to place the resulting format string
	 * @param f the format to use
	 * @return <code>buf</code>
	 * @throw InvalidFormatException if <code>f</code> is not a recognized format
	 */
	inline static const char *	format(const Time &t, char buf[TIMEBUF_SIZE], Time::DurationFormat f, Time::Zone tz)
	{
		return format(t, buf, static_cast<Time::Format>(f), tz);
	}
	
	/**
	 * Returns the difference between the specified times in whole days (24hr periods). The
	 * time portion of the Time is significant. ie diffAsDays(10/03/2006 10:00, 11/03/2006 09:59)
	 * returns zero because there is only 23hrs 59secs between the two times.
	 * 
	 * @param higher the higher of the two times
	 * @param lower the lower of the two times
	 * @return the number of whole days between the two times
	 */
	static int32 diffAsDays(const Time& higher, const Time& lower);

	/**
	 * @param higher the higher of the two times
	 * @param lower the lower of the two times
	 * @return the number of whole hours between the two times
	 */
	static int64 diffAsHours(const Time& higher, const Time& lower);

	/**
	 * @param higher the higher of the two times
	 * @param lower the lower of the two times
	 * @return the number of whole minutes between the two times
	 */
	static int64 diffAsMinutes(const Time& higher, const Time& lower);
	
	/**
	 * @param higher the higher of the two times
	 * @param lower the lower of the two times
	 * @return the number of whole seconds between the two times
	 */
	static int64 diffAsSeconds(const Time& higher, const Time& lower);
        
	/**
	 * @param higher the higher of the two times
	 * @param lower the lower of the two times
	 * @return the number of whole milliseconds between the two times
	 */
	static int64 diffAsMillis(const Time& higher, const Time& lower);
	
private:

	int64 m_time;
};

/**
 */
Time operator+(const Time &t1, const Time &t2);

/**
 */
Time operator-(const Time &t1, const Time &t2);

/**
 */
std::ostream & operator<<(std::ostream &os, const Time &t);


#endif // __COMMON_UTIL_TIME_HPP__

