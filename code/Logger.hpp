#ifndef __COMMON_LOGGING_LOGGER_HPP_
#define __COMMON_LOGGING_LOGGER_HPP_

#include <unordered_map>
#include "common.hpp"
#include "Mutex.hpp"
#include "atomic32.hpp"
#include "Thread.hpp"
#include "Condition.hpp"
#include "CircularBuffer.hpp"
// #include <common/util/Time.hpp>
#include <deque>
#include <map>
#include <stdio.h>
#include <boost/shared_ptr.hpp>

/** ----------------------------------------------------------------------------------
 * Define LOGGING_GROUP to override the default group name
 */
#ifdef LOGGING_GROUP
	#define DEFAULT_LOGGING_GROUP	TO_STRING( LOGGING_GROUP )
#else
	#define DEFAULT_LOGGING_GROUP	""
#endif

/**
 * Log to a particular group and level
 */
#define GLOG(group, level, msg, ...)     														\
    do {    																					\
		static Logger & logger = Logger::instance();	\
		static LogSettings & settings = 									\
				logger.createSettings(level, group, __LOCATION__, __FUNCTION_SIGNATURE__);      \
		if( settings.isLoggable() ) 															\
			logger.write( settings, msg , ##__VA_ARGS__ );                                      \
    } while( false )

#define IS_GLOGGABLE(group, level) ([]() -> bool \
	{ \
		static auto & settings = Logger::instance().createSettings(level, group, __LOCATION__, __FUNCTION_SIGNATURE__); \
		return settings.isLoggable(); \
	}())

#define IS_LOGGABLE(level) IS_GLOGGABLE(DEFAULT_LOGGING_GROUP, level)

/*
 * Limitless buffer on logging
 */
#define SLOG(level, group, msg)                                                                                   \
    do {                                                                                                          \
        static Logger &log = Logger::instance();                                                                  \
        static LogSettings &settings = log.createSettings(level, group, __LOCATION__, __FUNCTION_SIGNATURE__);    \
        if(settings.isLoggable())                                                                                 \
            log.write(settings, msg);                                                                             \
    } while( false );

/**
 * Log at the implicit level, using specified logging group
 */
#define GLTRCE(group, msg, ...) 	GLOG( group, LL_TRCE, msg , ##__VA_ARGS__ )
#define GLDBUG(group, msg, ...) 	GLOG( group, LL_DBUG, msg , ##__VA_ARGS__ )
#define GLINFO(group, msg, ...)  	GLOG( group, LL_INFO, msg , ##__VA_ARGS__ )
#define GLMETA(group, msg, ...)  	GLOG( group, LL_META, msg , ##__VA_ARGS__ )
#define GLWARN(group, msg, ...)  	GLOG( group, LL_WARN, msg , ##__VA_ARGS__ )
#define GLCRIT(group, msg, ...)  	GLOG( group, LL_CRIT, msg , ##__VA_ARGS__ )

/**
 * Log at the implicit level, using DEFAULT_LOGGING_GROUP
 */
#define LTRCE(msg, ...) 			GLTRCE( DEFAULT_LOGGING_GROUP, msg , ##__VA_ARGS__ )
#define LDBUG(msg, ...) 			GLDBUG( DEFAULT_LOGGING_GROUP, msg , ##__VA_ARGS__ )
#define LINFO(msg, ...)  			GLINFO( DEFAULT_LOGGING_GROUP, msg , ##__VA_ARGS__ )
#define LMETA(msg, ...)  			GLMETA( DEFAULT_LOGGING_GROUP, msg , ##__VA_ARGS__ )
#define LWARN(msg, ...)  			GLWARN( DEFAULT_LOGGING_GROUP, msg , ##__VA_ARGS__ )
#define LCRIT(msg, ...)  			GLCRIT( DEFAULT_LOGGING_GROUP, msg , ##__VA_ARGS__ )


// ----------------------------------------------------------------------------------- 
// ml::common::logging
// ----------------------------------------------------------------------------------- 

/** ----------------------------------------------------------------------------------
 * The logging levels available
 */
enum Level { LL_TRCE = 0,
			 LL_DBUG = 1, 
			 LL_INFO = 2,
			 LL_META = 3, 
			 LL_WARN = 4, 
			 LL_CRIT = 5 };

/** ----------------------------------------------------------------------------------
 * Convert a level to its string representation
 */
extern const char * levelToString(Level level);

/** ----------------------------------------------------------------------------------
 * Parse a string to its corresponding level
 */
extern Level stringToLevel(const char *str);

/** ----------------------------------------------------------------------------------
 * Class used by the GLOG macro to store the configuration details of a particular
 * log message.
 */
struct LogSettings
{						 
	inline bool isLoggable() 
	{
		return localLevel >= groupLevelRef;
	}
	
private:

	friend class Logger;

	LogSettings(const Level level, 
				const char *grp, 
				const char *loc, 
				const char *sig,
				atomic32 &glRef);

	const std::string group;
	std::string header;
	const Level localLevel;
	atomic32 &groupLevelRef;
};

/** ----------------------------------------------------------------------------------
 * The main loggerImpl class
 */
class Logger : private Thread
{
public:
	typedef std::map<std::string, std::string> StringMap;
	static Logger & instance();
	
public:

	bool testGroupLevel(Level lev, const std::string &group = DEFAULT_LOGGING_GROUP);
	void setGroupLevel(Level lev, const std::string &group = DEFAULT_LOGGING_GROUP);
	void startLoggingThread();
	void write(const LogSettings &settings, const char *format, ...)

#ifdef __GNUC__	
		// Tell the compiler that this follows printf syntax so it can do
		// some static format and argument checking. 
		// Note that format is actually the 3rd argument (taking into account the 'this' pointer)
		__attribute__ ((format (printf, 3, 4)))
#endif
	;

	// for writing large log messages
	void write(const LogSettings &settings, const std::string & message );

	/** Used by the GLOG macro to create a LogSettings object */
	LogSettings & createSettings(const Level level, 
								 const char *grp, 
								 const char *loc, 
								 const char *sig);

	void getAllGroups(StringMap & groups) const;

private:

	Logger();
	~Logger();

	/**
	 * Thread callback
	 */
	bool onProcess();
	
	// friend class mlc::application::Application;

	void setProgramName(const std::string & progName);
			
	void closeLogFiles();
                                 
    void flush();
			
	struct FileInfo;
	
private:
	
	typedef std::deque< boost::shared_ptr<LogSettings> > settings_deque;
	typedef std::unordered_map< std::string, atomic32 >	settings_map;
	typedef CircularBuffer< char > byte_buffer;
	
	enum FileType
	{
		FT_STDOUT,
		FT_STDERR,
		FT_FILE
	};
	
	
	
	settings_deque					m_settings;
	settings_map 					m_levels;
	byte_buffer						m_buffer;
	Mutex				m_mutex;
	Condition			m_cond;
	std::map<std::string, FileInfo> m_files;
	std::string						m_programName;
	uint32							m_counter;
};

#endif // __COMMON_LOGGING_LOGGER_HPP_
