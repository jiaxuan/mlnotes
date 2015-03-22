
#include <common/logging/Logger.hpp>
#include <common/exception/Exception.hpp>
#include <common/text/Format.hpp>
#include <common/util/Path.hpp>
#include <common/application/Application.hpp>
#include <common/registry/Registry.hpp>
#include <common/RegistryVariables.hpp>
#include <common/concurrent/gettid.hpp>

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syslog.h>
#include <linux/unistd.h>

using namespace ml::common;
using namespace ml::common::application;
using namespace ml::common::logging;
using namespace ml::common::util;
using namespace ml::common::registry;
using namespace ml::common::text;

// ----------------------------------------------------------------------------------- 
// ml::common::logging::Logger implementation
// ----------------------------------------------------------------------------------- 

namespace {

struct MessageHeader {
	util::Time now;
	uint32 size;
    mlc::concurrent::Thread::Identifier tid;
    pid_t thread_pid;
	const LogSettings *ps;
};

const uint32 THREAD_SLEEP_TIME_MILLIS		= 200;
const uint32 SLEEPS_TO_FLUSH				= 20;  //TIME IN ms = SLEEPS_TO_CHECK_REMOVAL * THREAD_SLEEP_TIME_MILLIS
const uint32 SLEEPS_TO_CHECK_FILE_REMOVAL	= 50;  //TIME IN ms = SLEEPS_TO_CHECK_FILE_REMOVAL * THREAD_SLEEP_TIME_MILLIS
const uint32 SLEEPS_TO_CHECK_LINK_REMOVAL	= 100; //TIME IN ms = SLEEPS_TO_CHECK_LINK_REMOVAL * THREAD_SLEEP_TIME_MILLIS

const uint32 MAX_FILE_SIZE 					= 100 * 1024 * 1024;
const uint32 MAX_BUFFER_SIZE 				= 16 * 1024 * 1024;
const uint32 LOGMSG_SIZE 					= 8192;
const uint32 LOGBUF_SIZE					= LOGMSG_SIZE + sizeof(MessageHeader);

const char * LEVEL_STRINGS[] 				= { "TRCE", "DBUG", "INFO", "META", "WARN", "CRIT" };
const byte LEVEL_COUNT 						= sizeof(LEVEL_STRINGS) / sizeof(LEVEL_STRINGS[0]);

const Level LL_DEFAULT						= LL_META;

bool logRemovedOrExceedsRollSize(FILE * file, std::string &name, const uint32 counter )
{
	static uint32 localcounter = 0;
	
	if( (counter - localcounter) < SLEEPS_TO_CHECK_FILE_REMOVAL)
	{
		return uint32( ftell( file)) >= MAX_FILE_SIZE;
	}
	else
	{
		struct stat st;
		
		localcounter = counter;
				
		if( 0 == stat(name.c_str(), &st) )
		{
			return uint32(st.st_size) >= MAX_FILE_SIZE;
		}
		else
		{
			return true;
		}
	}
}

bool linkRemoved(std::string &name, const uint32 counter)
{
	static uint32 localcounter = 0;
	
	if( (counter - localcounter) < SLEEPS_TO_CHECK_LINK_REMOVAL )
	{
		return false;
	}
	else
	{
		localcounter = counter;
		
		struct stat st;
	 	return  0 != lstat(name.c_str(), &st);
	}
}

RegKey & getEnvironmentKey()
{
	return Registry::instance().key(ENVIRONMENT_LOGGING_KEY, true);
}

RegKey & getEnvironmentGroupKey(const std::string &group)
{
	return getEnvironmentKey().key(group, true);
}

RegKey & getProcessKey()
{
	return Registry::instance().key(PROCESS_LOGGING_KEY, true);
}

RegKey & getProcessGroupKey(const std::string &group)
{
	return getProcessKey().key(group, true);
}

bool getDefaultSyslogEnabled()
{
	return Format::stringToBool(getProcessKey().value("syslog", getEnvironmentKey().value("syslog", "false")).c_str());
}

std::string getDefaultLevel()
{
	return getProcessKey().value("level", getEnvironmentKey().value("level", LEVEL_STRINGS[LL_DEFAULT]));
}

std::string getDefaultLocation()
{
	return getProcessKey().value("location", getEnvironmentKey().value("location", "<stdout>"));
}

std::string getGroupLevel(const std::string &grp)
{
	return getProcessGroupKey(grp).value("level",
				getEnvironmentGroupKey(grp).value("level", 
					getDefaultLevel()
					)
				);
}

std::string getGroupLocation(const std::string &grp)
{
	return getProcessGroupKey(grp).value("location",
				getEnvironmentGroupKey(grp).value("location", 
					getDefaultLocation()
					)
				);
}

}; // <internal> namespace


/** ----------------------------------------------------------------------------------
 */
struct Logger::FileInfo 
{
	FileInfo() : type(Logger::FT_FILE), file(NULL)
	{
	}
	
	~FileInfo()
	{
		if( file ) 
		{
			fflush(file);
			
			if( type == Logger::FT_FILE )
			{
				fclose(file);
			}
				
			file = NULL;
		}
	}

	FileType type;
	FILE * file;
	std::string name;
	std::string symName;
	char							fileBuffer[ 1024 * 100 ];
};


/** ----------------------------------------------------------------------------------
 */
Logger & Logger::instance()
{
	return mlc::application::Application::instance().logger();
}

/** ----------------------------------------------------------------------------------
 */
Logger::Logger() : 
	m_buffer( MAX_BUFFER_SIZE ),
	m_mutex(concurrent::Mutex::MT_Unchecked),
	m_programName( mlc::util::Path::baseName( mlc::util::Path::currentProcessPath().c_str() ) ),
	m_counter(0)
{
	// Note: we rely on the Application object to call Thread::start()
}

/** ----------------------------------------------------------------------------------
 */
Logger::~Logger()
{
	Thread::stop();
	Thread::join();
	
	flush();
	m_files.clear();
}

/** ----------------------------------------------------------------------------------
 */
void Logger::setProgramName(const std::string & progName)
{
	synchronized(m_mutex) 
	{
		m_programName = progName;
	}
}

void Logger::startLoggingThread() 
{
	start();
}

/** ----------------------------------------------------------------------------------
 */
bool Logger::testGroupLevel(Level lev, const std::string &group)
{
	bool flag = true;
	
	synchronized(m_mutex) 
	{
		concurrent::atomic32 & glRef = m_levels.insert( 
											settings_map::value_type(
														group, 
														concurrent::atomic32(LL_DEFAULT)
													) 
											).first->second;
		flag = lev >= glRef;
	}
	
	return flag;
}

/** ----------------------------------------------------------------------------------
 */
void Logger::setGroupLevel(Level lev, const std::string &group)
{
	// protect the hash_map
	synchronized(m_mutex) 
	{
		m_levels[group] = lev;
	}
}

/** ----------------------------------------------------------------------------------
 */
void Logger::write(const LogSettings &settings, const char *format, ...)
{
	static const bool useSyslog = getDefaultSyslogEnabled();

	char static_logbuf[LOGBUF_SIZE];
	char *logbuf = &static_logbuf[0];

	MessageHeader * mh = new(logbuf) MessageHeader();

	va_list ap;
	va_start(ap, format);

	mh->ps 	= &settings;
	mh->size = vsnprintf(logbuf + sizeof(MessageHeader), LOGMSG_SIZE, format, ap);
	mh->tid = mlc::concurrent::Thread::self();
	mh->thread_pid = concurrent::gettid();
	if( uint32(mh->size) >= LOGMSG_SIZE )
	{
		// Allocate enough to store the full data
		uint32 newMsgSize = mh->size + 1;
		boost::scoped_array<char> newbuf( new char[newMsgSize] );
		// Store the full data
		vsnprintf(newbuf.get(), newMsgSize, format, ap);
		// Only keep LOGMSG_SIZE - 1 worth of it
		mh->size = LOGMSG_SIZE - 1;
		memcpy(logbuf + sizeof(MessageHeader), newbuf.get(), mh->size);
	}

	va_end(ap);

	concurrent::Mutex::scoped_lock autolock(m_mutex);
	int32 result = m_buffer.write(logbuf, sizeof(MessageHeader) + mh->size);
	autolock.unlock();

	// If for some reason our buffer is full, inform the syslog and
	// make sure we don't lose CRIT messages
	if( useSyslog && byte_buffer::E_NOSPACE == result )
	{
		syslog(LOG_CRIT, "Internal log buffer is full: %s", 
							m_programName.c_str()
							);

		if( settings.localLevel == LL_CRIT )
			syslog(LOG_CRIT, "%s %s", settings.header.c_str(), logbuf + sizeof(MessageHeader));

	}

	// Make sure we process CRIT messages as quickly as possible
	if( settings.localLevel == LL_CRIT )
		m_cond.notify();
}

/** ----------------------------------------------------------------------------------
 */
void Logger::write(const LogSettings &settings, const std::string & message )
{
	static const bool useSyslog = getDefaultSyslogEnabled();

	MessageHeader mh;
	
	mh.ps 	= &settings;
	mh.size = message.size();
    mh.tid = mlc::concurrent::Thread::self();
    mh.thread_pid = concurrent::gettid();

	int32 result;
	concurrent::Mutex::scoped_lock autolock(m_mutex);
	if ( m_buffer.size() + mh.size + sizeof(MessageHeader) > m_buffer.capacity() )
	{
		result = byte_buffer::E_NOSPACE;
	}
	else
	{
		m_buffer.write(reinterpret_cast<char *>(&mh), sizeof(MessageHeader));
		result = m_buffer.write(message.data(), mh.size);
	}
	autolock.unlock();

	// If for some reason our buffer is full, inform the syslog and
	// make sure we don't lose CRIT messages
	if( useSyslog && byte_buffer::E_NOSPACE == result )
	{
		syslog(LOG_CRIT, "Internal log buffer is full: %s", 
							m_programName.c_str()
							);

		if( settings.localLevel == LL_CRIT )
			syslog(LOG_CRIT, "%s %s", settings.header.c_str(), message.c_str());
	}

	// Make sure we process CRIT messages as quickly as possible
	if( settings.localLevel == LL_CRIT )
		m_cond.notify();
}

/** ----------------------------------------------------------------------------------
 * Writes the messages to the associated log file
 * 
 * Can track file changes using: /usr/bin/tail --retry --follow=name <filename>
 */
bool Logger::onProcess()
{
	try
	{
		
		synchronized( m_mutex )
		{
			m_cond.wait(m_mutex, THREAD_SLEEP_TIME_MILLIS );
		}
		
			
		flush();
		m_counter++;
	}
	catch( std::exception & e ) 
	{
		if(getDefaultSyslogEnabled())
			syslog(LOG_CRIT, "%s - Logging Exception : %s ", 
								m_programName.c_str(),
								e.what()				);
	}
	catch( ... ) 
	{
		if(getDefaultSyslogEnabled())
			syslog(LOG_CRIT, "%s - Logging Unknown Exception ", m_programName.c_str());
	}
	
	return Thread::CONTINUE;
}

void Logger::closeLogFiles()
{
	flush();
	foreach( iter, m_files )
	{
		if( iter->second.file ) 
			fclose( iter->second.file );
	}
}

/** ----------------------------------------------------------------------------------
 */
void Logger::flush()
{
	static const bool useSyslog = getDefaultSyslogEnabled();

	char logbuf[LOGMSG_SIZE+1] = {0};
	char timebuf[Time::TIMEBUF_SIZE];
	MessageHeader mh;

	for(;;)
	{
		concurrent::Mutex::scoped_lock autolock(m_mutex);
		
		if( byte_buffer::E_NODATA == m_buffer.read(reinterpret_cast<char *>(&mh), sizeof(MessageHeader)) )
			break;
			
		bool log_immediate = mh.size <= LOGMSG_SIZE;
		
		if( log_immediate )
		{
			if( byte_buffer::E_NODATA == m_buffer.read(logbuf, mh.size) )
				break;

			logbuf[mh.size] = '\0';			
		}
		else
		{
			if( byte_buffer::E_NODATA == m_buffer.read(logbuf, LOGMSG_SIZE) )
				break;

			mh.size -= LOGMSG_SIZE;
			logbuf[LOGMSG_SIZE] = '\0';
		}
				
		autolock.unlock();

		if( useSyslog && mh.ps->localLevel == LL_CRIT )
		{
			syslog(LOG_CRIT, "%s %s", mh.ps->header.c_str(), logbuf);
		}
		
		FileInfo & fInfo = m_files[ mh.ps->group ];
		FILE * & file = fInfo.file;
		
		if( NULL == file || ( (fInfo.type == Logger::FT_FILE) && logRemovedOrExceedsRollSize(file, fInfo.name, m_counter) ) )
		{
			Time::format(mh.now, timebuf, Time::ISO_YYYYMMDDHHMMSS, Time::LOCAL);
			
			std::string filepath = getGroupLocation(mh.ps->group);
			
			if( file )
			{
				if( fInfo.type == Logger::FT_FILE )
					fclose( file );

				file = NULL;
			}

			if( filepath == "<stdout>" )
 			{
				fInfo.type 		= Logger::FT_STDOUT;
				fInfo.name 		= filepath;
				fInfo.symName 	= "";
				file 			= stdout;
			}
			else if( filepath == "<stderr>" )
			{
				fInfo.type 		= Logger::FT_STDERR;
				fInfo.name 		= filepath;
				fInfo.symName 	= "";				
				file 			= stderr;
			}
			else
			{
				fInfo.type = Logger::FT_FILE;
				
				fInfo.name = Format::toString(
										"%s/%s%s%s.%s",
										filepath.c_str(),
										m_programName.c_str(),
										mh.ps->group.size() > 0 ? "." : "",
										mh.ps->group.c_str(),
										timebuf);
	
				fInfo.symName  = Format::toString(
										"%s/%s%s%s.latest",
										filepath.c_str(),
										m_programName.c_str(),
										mh.ps->group.size() > 0 ? "." : "",
										mh.ps->group.c_str());

				// Make sure any old link is cleaned up
				unlink( fInfo.symName.c_str() );
					
				file = fopen(fInfo.name.c_str(), "a+");
				
				if( NULL != file )
				{
					setbuffer( file, fInfo.fileBuffer, sizeof( fInfo.fileBuffer));
					
					if(  0 != symlink(fInfo.name.c_str(), fInfo.symName.c_str())  && useSyslog)
						syslog(LOG_CRIT, "Could not create symlink: %s -> %s", fInfo.symName.c_str(), fInfo.name.c_str());
				}
			}
						
			if( NULL == file )
			{
				if (useSyslog)
					syslog(LOG_CRIT, "Could not open log file: %s", fInfo.name.c_str());

				continue;
			}
		}

		if( (fInfo.type == Logger::FT_FILE) && linkRemoved(fInfo.symName, m_counter ) )
		{
			// create the ".latest" symlink
			if( 0 != symlink(fInfo.name.c_str(), fInfo.symName.c_str()) )
			{
				switch( errno )
				{
				case EEXIST:
					// Link already exists. Replace it ...
					unlink( fInfo.symName.c_str() );
					
					if( 0 == symlink(fInfo.name.c_str(), fInfo.symName.c_str()) )
						break;
					// Fall through ...		
				default:
					if(useSyslog)
						syslog(LOG_CRIT, "Could not create symlink: %s -> %s", fInfo.symName.c_str(), fInfo.name.c_str());
				}
			}
		}
		
		Time::format(mh.now, timebuf, Time::YYYYMMDD_HHMMSSUU, Time::LOCAL);

		if (log_immediate)
		{
			fprintf(file, "[%s] %u/%lu - %s %s\n", timebuf, mh.thread_pid, mh.tid, mh.ps->header.c_str(), logbuf);
		}
		else
		{
			boost::scoped_array<char> heap_logbuf( new char[mh.size+1] ); 

			memset(heap_logbuf.get(), '\0', mh.size+1);
			
			concurrent::Mutex::scoped_lock autolock(m_mutex);

			if( byte_buffer::E_NODATA == m_buffer.read(heap_logbuf.get(), mh.size) )
				break;
								
			autolock.unlock();

			fprintf(file, "[%s] %u/%lu - %s %s%s\n", timebuf, mh.thread_pid, mh.tid, mh.ps->header.c_str(), logbuf, heap_logbuf.get());
		}	
	}

	if( (m_counter % SLEEPS_TO_FLUSH) == 0 )
	{
		foreach( iter, m_files ) 
		{
			if( iter->second.file ) 
				fflush( iter->second.file );
		}
	}
}

/** ----------------------------------------------------------------------------------
 */
LogSettings & Logger::createSettings(const Level level, const char *grp, const char *loc, const char *sig)
{
	if( grp == NULL || loc == NULL || sig == NULL )
		THROW(exception::InvalidArgumentException, "NULL string argument detected");

	Level defaultLevel = stringToLevel( getGroupLevel(grp).c_str() );
	
	concurrent::Mutex::scoped_lock autolock(m_mutex);
	concurrent::atomic32 & glRef = m_levels.insert( 
										settings_map::value_type(
													grp, 
													concurrent::atomic32(
															defaultLevel
															)
												) 
										).first->second;

	boost::shared_ptr<LogSettings> ps( new LogSettings(level, grp, loc, sig, glRef) );
	m_settings.push_back(ps);
	autolock.unlock();

	return * ps.get();
}

void Logger::getAllGroups(StringMap & groups) const
{
	synchronized(m_mutex)
	{
		foreach(iter, m_levels)
		{
			groups[iter->first] = levelToString(iter->second);
		}
	}
}

/** ----------------------------------------------------------------------------------
 */
LogSettings::LogSettings(const Level level, const char *grp, const char *loc, const char *sig, concurrent::atomic32 &glRef) :
				group(grp),
				localLevel(level),
				groupLevelRef(glRef)
{
	header = Format::toString(
					"%4s %s {{%s}} :",
					levelToString(localLevel),
					Path::baseName(loc).c_str(),
					sig
				);
}

/** ----------------------------------------------------------------------------------
 */
const char * logging::levelToString(Level level)
{	
	if( level >= LEVEL_COUNT )
		THROW(exception::InvalidArgumentException, "Unknown log level");

	return LEVEL_STRINGS[level];
}

/** ----------------------------------------------------------------------------------
 */
Level logging::stringToLevel(const char *str)
{
	if( NULL == str )
		THROW(exception::InvalidArgumentException, "NULL log level string");

	for(byte i=0; i<LEVEL_COUNT; ++i)
		if( 0 == strcasecmp(str, LEVEL_STRINGS[i]) )
			return Level(i);

	THROW(exception::InvalidArgumentException, "Invalid log level string (" + std::string(str) + ")");
}

